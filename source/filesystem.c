#include <bulge/bulge.h>

#define VERSION_MAX 0x00010000
#define ROOT_RESERVED_BLOCKS 8 /* enough for 32 entries */

BulgeError bulgeFilesystem_create(BulgeFilesystem* filesystem, uint32_t block_begin, uint32_t block_count, bulgeBlockRead read, bulgeBlockWrite write, void* callback_data, uint8_t* block_buffer) {
  if(block_count < 8) { return BULGE_ERROR_TOO_SMALL; }
  if(block_count < (2 + ROOT_RESERVED_BLOCKS)) { return BULGE_ERROR_TOO_SMALL; }

  // we'll need these stored early-on (the open call later will overwrite them)
  filesystem->block_filesystem = block_begin;
  filesystem->blocks_total     = block_count;
  filesystem->read             = read;
  filesystem->write            = write;
  filesystem->callback_data    = callback_data;

  // build & write header
  BulgeHeader* header = (BulgeHeader*)block_buffer;
  header->signature[0] = 'B';
  header->signature[1] = 'u';
  header->signature[2] = 'l';
  header->signature[3] = 'g';
  header->signature[4] = 'e';
  header->signature[5] = 'F';
  header->signature[6] = 'S';
  header->signature[7] = '!';
  header->version           = BULGE_ENDIAN32(VERSION_MAX);
  header->block_count_total = BULGE_ENDIAN32(block_count);
  header->block_count_table = BULGE_ENDIAN32(block_count >> 7); // blocks for table = blocks_total * 4 / 512 == blocks_total / 128 == blocks_total >> 7
  header->blocks_occupied   = BULGE_ENDIAN32(1 + BULGE_ENDIAN32(header->block_count_table) + ROOT_RESERVED_BLOCKS); // 1 for header + block_table + x preallocated for root directory
  header->blocks_free       = BULGE_ENDIAN32(block_count - BULGE_ENDIAN32(header->blocks_occupied));
  header->root_block        = BULGE_ENDIAN32(1 + BULGE_ENDIAN32(header->block_count_table));
  for(uint8_t idx=0; idx< 32; ++idx) { header->system[idx] = 0x00; }
  for(uint8_t idx=0; idx< 16; ++idx) { header->uuid[idx]   = 0x00; }
  for(uint8_t idx=0; idx<128; ++idx) { header->name[idx]   = 0x00; }
  if(!write(block_begin, block_buffer, callback_data)) { return BULGE_ERROR_DEVICE_WRITE; }

  // store some information since we're about to overwrite data in buffer
  BulgeError error;
  uint32_t root_block = BULGE_ENDIAN32(header->root_block);
  uint32_t count_blocks_table  = BULGE_ENDIAN32(header->block_count_table);

  // to start with, mark entire chain table as free
  BulgeTableAccessor table_accessor;
  bulgeTableAccessor_reset(&table_accessor, block_buffer);
  for(uint32_t idx=0; idx<block_count; ++idx) {
    error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, idx, BULGE_TABLE_FREE);
    if(error > BULGE_ERROR_NONE) { return error; }
  }

  // reserve 1 block for header
  error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, 0, BULGE_TABLE_FINAL);
  if(error > BULGE_ERROR_NONE) { return error; }
  // reserve blocks for table itself; each block points to the next, except for the last
  for(uint32_t idx=0; idx<count_blocks_table; ++idx) {
    error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, idx+1, idx+2);
    if(error > BULGE_ERROR_NONE) { return error; }
  }
  error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, 1+count_blocks_table-1, BULGE_TABLE_FINAL);
  if(error > BULGE_ERROR_NONE) { return error; }
  // reserve blocks for root directory
  for(uint32_t idx=0; idx<ROOT_RESERVED_BLOCKS; ++idx) {
    error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, 1+count_blocks_table+idx, 1+count_blocks_table+idx+1);
    if(error > BULGE_ERROR_NONE) { return error; }
  }
  error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, 1+count_blocks_table+ROOT_RESERVED_BLOCKS-1, BULGE_TABLE_FINAL);
  if(error > BULGE_ERROR_NONE) { return error; }
  error = bulgeTableAccessor_flush(filesystem, &table_accessor);
  if(error > BULGE_ERROR_NONE) { return error; }

  // now, write root directory
  for(uint32_t idx=0; idx<512; ++idx) { block_buffer[idx] = 0x00; } // zero out buffer
  BulgeEntryAccessor entry_accessor;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
  BulgeEntry* entry;
  // first, for ease, mark all directory entries as free
  uint32_t max_root_entries = ROOT_RESERVED_BLOCKS << 2; // 4 entries per block
  for(uint32_t idx=0; idx<max_root_entries; ++idx) {
    error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, root_block, idx, &entry);
    if(error > BULGE_ERROR_NONE) { return error; }
    entry->type = BULGE_ENTRY_TYPE_FREE;
    bulgeEntryAccessor_setDirty(&entry_accessor);
  }

  // create the root directory self entry
  BulgeEntrySelf* root;
  error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, root_block, 0, (BulgeEntry**)(&root));
  if(error > BULGE_ERROR_NONE) { return error; }
  root->type                  = BULGE_ENTRY_TYPE_SELF;
  root->root_user             = 0;
  root->root_group            = 0;
  root->root_permissions      = 0x3F; // --rwrwrw
  root->block_self            = BULGE_ENDIAN32(root_block);
  root->block_parent          = 0;
  root->root_size_blocks      = BULGE_ENDIAN32(ROOT_RESERVED_BLOCKS);
  root->root_time_created[0]  = 0;
  root->root_time_created[1]  = 0;
  root->root_time_modified[0] = 0;
  root->root_time_modified[1] = 0;
  root->root_attribute_block  = 0;
  root->root_attribute_size   = 0;
  root->root_system_flags     = 0;
  root->name[0]               = 'r';
  root->name[1]               = 'o';
  root->name[2]               = 'o';
  root->name[3]               = 't';
  root->name[4]               = 0x00;
  bulgeEntryAccessor_setDirty(&entry_accessor);
  error = bulgeEntryAccessor_flush(filesystem, &entry_accessor);
  if(error > BULGE_ERROR_NONE) { return error; }

  return bulgeFilesystem_open(filesystem, block_begin, read, write, callback_data, block_buffer);
}

BulgeError bulgeFilesystem_open(BulgeFilesystem* filesystem, uint32_t block, bulgeBlockRead read, bulgeBlockWrite write, void* callback_data, uint8_t* block_buffer) {
  if(!read(block, block_buffer, callback_data)) { return BULGE_ERROR_DEVICE_READ; }
  
  BulgeHeader* header = (BulgeHeader*)block_buffer;
  if(header->signature[0] != 'B') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(header->signature[1] != 'u') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(header->signature[2] != 'l') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(header->signature[3] != 'g') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(header->signature[4] != 'e') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(header->signature[5] != 'F') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(header->signature[6] != 'S') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(header->signature[7] != '!') { return BULGE_ERROR_NOT_BULGE_FS; }
  if(BULGE_ENDIAN32(header->version) > VERSION_MAX) { return BULGE_ERROR_FS_VERSION_TOO_NEW; }

  filesystem->block_filesystem = block;
  filesystem->block_table      = block + 1;
  filesystem->block_root       = block + BULGE_ENDIAN32(header->block_count_table) + 1;
  filesystem->blocks_total     = BULGE_ENDIAN32(header->block_count_total);
  filesystem->blocks_occupied  = BULGE_ENDIAN32(header->blocks_occupied);
  filesystem->blocks_free      = BULGE_ENDIAN32(header->blocks_free);
  for(uint8_t idx=0; idx<32; ++idx) { filesystem->system[idx] = header->system[idx]; }
  filesystem->read  = read;
  filesystem->write = write;
  filesystem->callback_data = callback_data;

  return BULGE_ERROR_NONE;
}

BulgeError bulgeFilesystem_close(BulgeFilesystem* filesystem, uint8_t* block_buffer) {
  if(!filesystem->read(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_READ; }
  
  BulgeHeader* header = (BulgeHeader*)block_buffer;
  header->blocks_occupied = BULGE_ENDIAN32(filesystem->blocks_occupied);
  header->blocks_free     = BULGE_ENDIAN32(filesystem->blocks_free);
  for(uint8_t idx=0; idx<32; ++idx) {
    header->system[idx] = filesystem->system[idx];
  }

  if(!filesystem->write(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_WRITE; }
  return BULGE_ERROR_NONE;
}

BulgeError bulgeFilesystem_getInformation(BulgeFilesystem* filesystem, BulgeFilesystemInformation* information, uint8_t* block_buffer) {
  if(!filesystem->read(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_READ; }
  
  BulgeHeader* header = (BulgeHeader*)block_buffer;
  information->version      = BULGE_ENDIAN32(header->version);
  information->blocks_total = BULGE_ENDIAN32(header->block_count_total);
  information->blocks_free  = filesystem->blocks_free;
  for(uint8_t idx=0; idx< 32; ++idx) { information->system[idx] = filesystem->system[idx]; }
  for(uint8_t idx=0; idx< 16; ++idx) { information->uuid[idx]   = header->uuid[idx];       }
  for(uint8_t idx=0; idx<128; ++idx) { information->name[idx]   = header->name[idx];       }

  return BULGE_ERROR_NONE;
}

BulgeError bulgeFilesystem_setId(BulgeFilesystem* filesystem, uint8_t* uuid, uint8_t* block_buffer) {
  if(!filesystem->read(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_READ; }
  
  BulgeHeader* header = (BulgeHeader*)block_buffer;
  for(uint8_t idx=0; idx<16; ++idx) { header->uuid[idx] = uuid[idx]; }

  if(!filesystem->write(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_WRITE; }
  return BULGE_ERROR_NONE;
}

BulgeError bulgeFilesystem_setName(BulgeFilesystem* filesystem, uint8_t* name, uint8_t* block_buffer) {
  if(!filesystem->read(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_READ; }
  
  BulgeHeader* header = (BulgeHeader*)block_buffer;
  for(uint8_t idx=0; idx<128; ++idx) {
    if(*name) {
      header->name[idx] = *name;
      ++name;
    } else {
      header->name[idx] = 0x00;
    }
  }

  if(!filesystem->write(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_WRITE; }
  return BULGE_ERROR_NONE;
}

BulgeError bulgeFilesystem_setSystemData(BulgeFilesystem* filesystem, uint8_t* system, uint8_t* block_buffer) {
  if(!filesystem->read(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_READ; }
  
  BulgeHeader* header = (BulgeHeader*)block_buffer;
  for(uint8_t idx=0; idx<32; ++idx) { header->system[idx] = system[idx]; }

  if(!filesystem->write(filesystem->block_filesystem, block_buffer, filesystem->callback_data)) { return BULGE_ERROR_DEVICE_WRITE; }
  return BULGE_ERROR_NONE;
}

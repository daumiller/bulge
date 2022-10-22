#include <bulge/bulge.h>

void bulgeEntryAccessor_reset(BulgeEntryAccessor* accessor, uint8_t* block_buffer) {
  accessor->block_directory_current = 0;
  accessor->block_entry_index       = 0;
  accessor->block_current           = 0;
  accessor->block_buffer            = block_buffer;
  accessor->dirty                   = false;
}

void bulgeEntryAccessor_setDirty(BulgeEntryAccessor* accessor) {
  accessor->dirty = true;
}

BulgeError bulgeEntryAccessor_flush(BulgeFilesystem* filesystem, BulgeEntryAccessor* accessor) {
  if(filesystem->write(filesystem->block_filesystem + accessor->block_current, accessor->block_buffer, filesystem->callback_data) == false) {
    return BULGE_ERROR_DEVICE_WRITE;
  }

  accessor->dirty = false;
  return BULGE_ERROR_NONE;
}

BulgeError bulgeEntryAccessor_getEntry(BulgeFilesystem* filesystem, BulgeEntryAccessor* accessor, uint32_t directory_block, uint32_t directory_index, BulgeEntry** entry_pointer) {
  bool needs_load = false;
  BulgeError error;

  if(directory_block != accessor->block_directory_current) { needs_load = true; }
  if(directory_index  < accessor->block_entry_index      ) { needs_load = true; }
  if(directory_index  > accessor->block_entry_index+3    ) { needs_load = true; } // 4 entries per block (128*4=512)

  if(needs_load) {
    // if dirty, flush
    if(accessor->dirty) {
      error = bulgeEntryAccessor_flush(filesystem, accessor);
      if(error > BULGE_ERROR_NONE) { return error; }
    }

    // first, we need to know how many blocks in this index is
    uint32_t block_offset = directory_index >> 2; // 4 entries per block
    accessor->block_current = directory_block;
    
    // now, we need to get the block_offset-th block, starting from directory_block
    BulgeTableAccessor table_accessor;
    bulgeTableAccessor_reset(&table_accessor, accessor->block_buffer);
    for(uint32_t idx=0; idx<block_offset; ++idx) {
      error = bulgeTableAccessor_getEntry(filesystem, &table_accessor, accessor->block_current, &(accessor->block_current));
      if(error > BULGE_ERROR_NONE) { return error; }
    }
    if(accessor->block_current == BULGE_TABLE_FREE ) { return BULGE_ERROR_CORRUPTED_DIRECTORY; } // directory corrupted
    if(accessor->block_current == BULGE_TABLE_FINAL) { return BULGE_ERROR_END_OF_DIRECTORY;    } // index out of bounds

    // now we need to read that block into our buffer
    if(filesystem->read(filesystem->block_filesystem + accessor->block_current, accessor->block_buffer, filesystem->callback_data) == false) {
      return BULGE_ERROR_DEVICE_READ;
    }

    // and setup the rest of our struct values
    accessor->block_directory_current = directory_block;
    accessor->block_entry_index       = directory_index & 0xFFFFFFFC; // first entry = directory_index - (directory_index % 4)
  }

  *entry_pointer = &(((BulgeEntry*)(accessor->block_buffer))[directory_index - accessor->block_entry_index]);
  return BULGE_ERROR_NONE;
}

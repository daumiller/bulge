#include <bulge/bulge.h>
#include <stdio.h>

#define TEST_FILE_PATH "bulge.img"
#define TEST_FILE_SIZE (1024*1024)
static FILE* test_file = NULL;
static uint8_t block_buffer[512];

bool blockRead(uint32_t block, uint8_t* buffer, void* data) {
  fseek(test_file, (block << 9), SEEK_SET);

  size_t read = 0;
  while(read < 512) {
    size_t this_read = fread(buffer + read, 1, 512-read, test_file);
    if(this_read == 0) { return false; }
    read += this_read;
  }

  return true;
}

bool blockWrite(uint32_t block, uint8_t* buffer, void* data) {
  fseek(test_file, (block << 9), SEEK_SET);

  size_t wrote = 0;
  while(wrote < 512) {
    size_t this_write = fwrite(buffer + wrote, 1, 512-wrote, test_file);
    if(this_write == 0) { return false; }
    wrote += this_write;
  }

  return true;
}

void zeroTestFile() {
  fseek(test_file, 0, SEEK_SET);
  for(uint16_t idx=0; idx<512; ++idx) {
    block_buffer[idx] = 0;
  }
  uint32_t block_count = TEST_FILE_SIZE >> 9;
  for(uint32_t idx=0; idx<block_count; ++idx) {
    blockWrite(idx, block_buffer, NULL);
  }
  fseek(test_file, 0, SEEK_SET);
}

void dumpEntry(BulgeEntry* entry) {
  printf("           Type  : %u\n", entry->type);
  printf("           User  : %u\n", entry->user);
  printf("           Group : %u\n", entry->group);
  printf("     Permissions : %u\n", entry->permissions);
  printf("  Starting Block : %u\n", BULGE_ENDIAN32(entry->block));
  printf("   Size in Bytes : %u\n", BULGE_ENDIAN32(entry->size_bytes));
  printf("  Size in Blocks : %u\n", BULGE_ENDIAN32(entry->size_blocks));
  printf("            Name : \"%s\"\n", (const char*)(entry->name));
}

static void dumpDirectory(BulgeFilesystem* fs, uint32_t directory_block, uint32_t indentation, uint8_t* block_buffer) {
  // list everything in directory
  BulgeError error;
  BulgeEntry* entry = NULL;
  BulgeEntryAccessor entry_accessor;
  uint32_t entry_index = 0;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
  while(true) {
    error = bulgeEntryAccessor_getEntry(fs, &entry_accessor, directory_block, entry_index, &entry);
    if(error == BULGE_ERROR_END_OF_DIRECTORY) { break; }
    if(error != BULGE_ERROR_NONE) {
      printf("  Error reading directory: %s\n", bulgeError_string(error));
      break;
    }
    ++entry_index;

    bool is_directory = false;
    if((entry->type >= BULGE_ENTRY_TYPE_FILE_LOWER) && (entry->type <= BULGE_ENTRY_TYPE_FILE_UPPER)) {
    } else if((entry->type >= BULGE_ENTRY_TYPE_DIRECTORY_LOWER) && (entry->type <= BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
      is_directory = true;
    } else {
      continue;
    }

    for(uint32_t idx=0; idx<indentation; ++idx) { printf(" "); }
    for(uint8_t idx=0; idx<80; ++idx) {
      if(entry->name[idx] == 0) { break; }
      printf("%c", (char)(entry->name[idx]));
    }
    if(is_directory) { printf("/"); }
    printf("\n");

    if(is_directory) {
      dumpDirectory(fs, BULGE_ENDIAN32(entry->block), indentation+2, block_buffer);
      bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
    }
  }
}

// in preparation, `dd if=/dev/zero of=./bulge.img bs=1m count=1`
int main(int argc, char** argv) {
  test_file = fopen(TEST_FILE_PATH, "r+");
  if(!test_file) {
    fprintf(stderr, "Error opening test file.\n");
    return -1;
  }
  // zeroTestFile();

  BulgeFilesystem fs;
  BulgeError error;
  error = bulgeFilesystem_create(&fs, 0, 2048, blockRead, blockWrite, NULL, block_buffer);
  if(error > BULGE_ERROR_NONE) {
    fprintf(stderr, "bulgeFilesystem_create failed: %s\n", bulgeError_string(error));
    fclose(test_file);
    return -1;
  }

  uint8_t uuid[] = { 0x13, 0x37, 0xBA, 0xBE, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xF0, 0x0D, 0x11, 0x22, 0x33, 0x44 };
  error = bulgeFilesystem_setId(&fs, uuid, block_buffer);
  if(error > BULGE_ERROR_NONE) { fprintf(stderr, "bulgeFilesystem_setId failed: %s\n", bulgeError_string(error)); }

  char* name = "Bulge Testing Filesystem";
  error = bulgeFilesystem_setName(&fs, (uint8_t*)name, block_buffer);
  if(error > BULGE_ERROR_NONE) { fprintf(stderr, "bulgeFilesystem_setName failed: %s\n", bulgeError_string(error)); }

  BulgeFilesystemInformation info;
  error = bulgeFilesystem_getInformation(&fs, &info, block_buffer);
  if(error > BULGE_ERROR_NONE) {
    fprintf(stderr, "bulgeFilesystem_getInformation failed: %s\n", bulgeError_string(error));
  } else {
    uint64_t bytes_total = ((uint64_t)info.blocks_total) << 9;
    uint64_t bytes_free  = ((uint64_t)info.blocks_free ) << 9;
    printf("  Version      : %08X\n",   info.version);
    printf("  Blocks Total : %u\n",     info.blocks_total);
    printf("  Blocks Free  : %u\n",     info.blocks_free);
    printf("  Name         : \"%s\"\n", (const char*)&(info.name));
    printf("  Bytes Total  : %llu\n",    bytes_total);
    printf("  Bytes Free   : %llu\n",    bytes_free);
  }

  // uint32_t directory_block;
  // printf("\"/\"\n"); bulgePath_openDirectory(&fs, "/", block_buffer, &directory_block);
  // printf("\"/hello\"\n"); bulgePath_openDirectory(&fs, "/hello", block_buffer, &directory_block);
  // printf("\"/hello/world\"\n"); bulgePath_openDirectory(&fs, "/hello/world", block_buffer, &directory_block);
  // printf("\"/bulge/test/file.text\"\n"); bulgePath_openDirectory(&fs, "/bulge/test/file.text", block_buffer, &directory_block);
  // printf("\"/eyo/\"\n"); bulgePath_openDirectory(&fs, "/eyo/", block_buffer, &directory_block);
  // printf("\"//\"\n"); bulgePath_openDirectory(&fs, "//", block_buffer, &directory_block);
  // printf("\"/eyo/lego/\"\n"); bulgePath_openDirectory(&fs, "/eyo/lego/", block_buffer, &directory_block);

  // BulgeEntry* entry = NULL;
  // printf("\"/\"\n");
  // error = bulgePath_findEntry(&fs, (uint8_t*)"", block_buffer, &entry, 0, 0);
  // if(error > BULGE_ERROR_NONE) {
  //   printf("  File not found...\n");
  // } else {
  //   dumpEntry(entry);
  // }

  BulgeEntry create_entry;

  // create a diretory
  create_entry.type             = BULGE_ENTRY_TYPE_DIRECTORY_LOWER + 3;
  create_entry.user             = 8;
  create_entry.group            = 1;
  create_entry.permissions      = 7;
  create_entry.time_created[0]  = 0;
  create_entry.time_created[1]  = 0;
  create_entry.time_modified[0] = 0;
  create_entry.time_modified[1] = 0;
  create_entry.system_flags     = 0x1337;
  error = bulgePath_createDirectory(&fs, (uint8_t *)"/development", &create_entry, block_buffer);
  if(error > BULGE_ERROR_NONE) {
    printf("Error creating directory: %s\n", bulgeError_string(error));
  }

  // create a file
  create_entry.type         = BULGE_ENTRY_TYPE_FILE_LOWER;
  create_entry.system_flags = 0xBABE;
  error = bulgePath_createFile(&fs, (uint8_t *)"/boot.script", &create_entry, block_buffer);
  if(error > BULGE_ERROR_NONE) {
    printf("Error creating file: %s\n", bulgeError_string(error));
  }

  // create another file
  create_entry.type         = BULGE_ENTRY_TYPE_FILE_LOWER;
  create_entry.system_flags = 0xDEAD;
  error = bulgePath_createFile(&fs, (uint8_t *)"/development/hello_world.c", &create_entry, block_buffer);
  if(error > BULGE_ERROR_NONE) {
    printf("Error creating file: %s\n", bulgeError_string(error));
  }

  // list filesystem contents
  printf("/\n");
  dumpDirectory(&fs, fs.block_root, 2, block_buffer);

  bulgeFilesystem_close(&fs, block_buffer);
  fclose(test_file);
  return 0;
}

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "storage-types.h"
#include "filesystem.h"
#include "path.h"

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

// in preparation, `dd if=/dev/zero of=./bulge.img bs=1m count=1`
int main(int argc, char** argv) {
  test_file = fopen(TEST_FILE_PATH, "r+");
  if(!test_file) {
    fprintf(stderr, "Error opening test file.\n");
    return -1;
  }
  zeroTestFile();

  BulgeFilesystem fs;
  if(bulgeFilesystem_create(&fs, 0, 2048, blockRead, blockWrite, NULL, block_buffer) == false) {
    fprintf(stderr, "bulgeFilesystem_create failed.\n");
    fclose(test_file);
    return -1;
  }

  uint8_t uuid[] = { 0x13, 0x37, 0xBA, 0xBE, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xF0, 0x0D, 0x11, 0x22, 0x33, 0x44 };
  if(bulgeFilesystem_setId(&fs, uuid, block_buffer) == false) { fprintf(stderr, "bulgeFilesystem_setId failed.\n"); }

  char* name = "Bulge Testing Filesystem";
  if(bulgeFilesystem_setName(&fs, (uint8_t*)name, block_buffer) == false) { fprintf(stderr, "bulgeFilesystem_setName failed.\n"); }

  BulgeFilesystemInformation info;
  if(bulgeFilesystem_getInformation(&fs, &info, block_buffer) == false) {
    fprintf(stderr, "bulgeFilesystem_getInformation failed.\n");
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

  BulgeEntry* entry = NULL;
  printf("\"/\"\n");
  if(bulgePath_findEntry(&fs, (uint8_t*)"", block_buffer, &entry) == false) {
    printf("  File not found...\n");
  } else {
    dumpEntry(entry);
  }

  bulgeFilesystem_close(&fs, block_buffer);
  fclose(test_file);
  return 0;
}

#ifndef BULGE_PATH_HEADER
#define BULGE_PATH_HEADER

#include <stdint.h>
#include <stdbool.h>
#include "storage-types.h"
#include "filesystem.h"

// NOTE: on return, entry_pointer will be set to point to data within the given block_buffer
bool bulgePath_findEntry(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, BulgeEntry** entry_pointer);

bool bulgePath_openDirectory(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_entry_block);
bool bulgePath_openFile(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_data_block);

bool bulgePath_openDirectoryAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_attribute_block);
bool bulgePath_openFileAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_attribute_block);

#endif // BULGE_PATH_HEADER

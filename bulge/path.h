#ifndef BULGE_PATH_HEADER
#define BULGE_PATH_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <bulge/error.h>
#include <bulge/storage-types.h>
#include <bulge/filesystem.h>

// NOTE: on return, entry_pointer will be set to point to data within the given block_buffer
BulgeError bulgePath_findEntry(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, BulgeEntry** entry_pointer);

BulgeError bulgePath_openDirectory(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_entry_block);
BulgeError bulgePath_openFile(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_data_block);

BulgeError bulgePath_openDirectoryAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_attribute_block);
BulgeError bulgePath_openFileAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_attribute_block);

#endif // BULGE_PATH_HEADER

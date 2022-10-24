#ifndef BULGE_PATH_HEADER
#define BULGE_PATH_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <bulge/error.h>
#include <bulge/storage-types.h>
#include <bulge/filesystem.h>

BulgeError bulgePath_findParent(BulgeFilesystem* filesystem, uint8_t* path, uint32_t* parent_block, uint8_t* block_buffer, uint8_t** name_beings);

// NOTE: on return, entry_pointer will be set to point to data within the given block_buffer
BulgeError bulgePath_findEntry(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, BulgeEntry** entry_pointer, uint32_t* parent_block, uint32_t* parent_index);
// NOTE: be sure that modified_entry is not within block_buffer before calling (copy data from findEntry() somewhere else before modifying)
BulgeError bulgePath_modifyEntry(BulgeFilesystem* filesystem, BulgeEntry* modified_entry, uint32_t parent_block, uint32_t parent_index, uint8_t* block_buffer);

BulgeError bulgePath_createDirectory (BulgeFilesystem* filesystem, uint8_t* path, BulgeEntry* entry, uint8_t* block_buffer);
BulgeError bulgePath_createFile      (BulgeFilesystem* filesystem, uint8_t* path, BulgeEntry* entry, uint8_t* block_buffer);
BulgeError bulgePath_createAttributes(BulgeFilesystem* filesystem, uint8_t* path, BulgeEntry* entry, uint8_t* block_buffer);

BulgeError bulgePath_openDirectory (BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_entry_block);
BulgeError bulgePath_openFile      (BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_data_block);
BulgeError bulgePath_openAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* attribute_block);

#endif // BULGE_PATH_HEADER

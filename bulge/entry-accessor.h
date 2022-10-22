#ifndef BULGE_ENTRY_ACCESSOR_HEADER
#define BULGE_ENTRY_ACCESSOR_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <bulge/error.h>
#include <bulge/storage-types.h>
#include <bulge/filesystem.h>

// NOTE!!!
//   BulgeEntryAccessor assumes that you will not modify block_buffer in between alls (unless you reset/flush)
//   if, during read-only operations, you need to use block_buffer for something else, you must call reset before using again
//   if, during writing operations, you need to use block_buffer for something else, you must call flush before modifying block_buffer

typedef struct structBulgeEntryAccessor {
  uint32_t block_directory_current; // starting block of current directory
  uint32_t block_entry_index;       // what index the current block starts at
  uint32_t block_current;           // current_block, relative to beginning of filesystem
  uint8_t* block_buffer;            // read/write cache buffer
  bool     dirty;                   // whether data has been wrote to a getEntry result
} BulgeEntryAccessor;

void bulgeEntryAccessor_reset   (BulgeEntryAccessor* accessor, uint8_t* block_buffer);
void bulgeEntryAccessor_setDirty(BulgeEntryAccessor* accessor);
BulgeError bulgeEntryAccessor_flush   (BulgeFilesystem* filesystem, BulgeEntryAccessor* accessor);
BulgeError bulgeEntryAccessor_getEntry(BulgeFilesystem* filesystem, BulgeEntryAccessor* accessor, uint32_t directory_block, uint32_t directory_index, BulgeEntry** entry_pointer);

#endif // ifndef BULGE_ENTRY_ACCESSOR_HEADER

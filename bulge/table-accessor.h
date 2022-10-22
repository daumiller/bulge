#ifndef BULGE_TABLE_ACCESSOR_HEADER
#define BULGE_TABLE_ACCESSOR_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <bulge/error.h>
#include <bulge/filesystem.h>

// NOTE!!!
//   BulgeTableAccessor assumes that you will not modify block_buffer in between alls (unless you reset/flush)
//   if, during getEntry operations, you need to use block_buffer for something else, you must call reset before using again
//   if, during setEntry operations, you need to use block_buffer for something else, you must call flush before modifying block_buffer

typedef struct structBulgeTableAccessor {
  uint32_t block_current;  // current_block, relative to beginning of filesystem
  uint8_t* block_buffer;   // read/write cache buffer
  bool dirty;
} BulgeTableAccessor;

void       bulgeTableAccessor_reset   (BulgeTableAccessor* accessor, uint8_t* block_buffer);
BulgeError bulgeTableAccessor_flush   (BulgeFilesystem* filesystem, BulgeTableAccessor* accessor);
BulgeError bulgeTableAccessor_getEntry(BulgeFilesystem* filesystem, BulgeTableAccessor* accessor, uint32_t table_index, uint32_t* table_value);
BulgeError bulgeTableAccessor_setEntry(BulgeFilesystem* filesystem, BulgeTableAccessor* accessor, uint32_t table_index, uint32_t table_value);

#endif // ifndef BULGE_TABLE_ACCESSOR_HEADER

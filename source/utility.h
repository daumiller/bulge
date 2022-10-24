#ifndef BULGE_UTILITY_HEADER
#define BULGE_UTILITY_HEADER

#include <bulge/bulge.h>

BulgeError bulgeUtility_findFreeBlock(BulgeFilesystem* filesystem, uint32_t* block, uint8_t* block_buffer);
BulgeError bulgeUtility_growChain    (BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* block_buffer);
BulgeError bulgeUtility_findEmptyEntry(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* block_buffer, BulgeEntry** entry_pointer, uint32_t* index_pointer);
BulgeError bulgeUtility_findNamedEntry(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* name_begin, uint8_t* name_end, uint8_t* block_buffer, BulgeEntry** entry_pointer, uint32_t* index_pointer);

#endif // ifndef BULGE_UTILITY_HEADER

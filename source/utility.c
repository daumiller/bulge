#include "utility.h"

BulgeError bulgeUtility_findFreeBlock(BulgeFilesystem* filesystem, uint32_t* block, uint8_t* block_buffer) {
  BulgeTableAccessor table_accessor;
  bulgeTableAccessor_reset(&table_accessor, block_buffer);

  uint32_t block_current = 0;
  uint32_t block_maximum = filesystem->blocks_total - 1;
  uint32_t block_current_value;
  while(block_current < block_maximum) {
    BulgeError error = bulgeTableAccessor_getEntry(filesystem, &table_accessor, block_current, &block_current_value);
    if(error > BULGE_ERROR_NONE) { return error; }
    if(block_current_value == BULGE_TABLE_FREE) {
      *block = block_current;
      return BULGE_ERROR_NONE;
    }
    ++block_current;
  }

  return BULGE_ERROR_FULL_FILESYSTEM;
}

BulgeError bulgeUtility_growDirectory(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* block_buffer) {
  // get a free block
  uint32_t block_free;
  BulgeError error = bulgeUtility_findFreeBlock(filesystem, &block_free, block_buffer);
  if(error > BULGE_ERROR_NONE) { return error; }

  BulgeTableAccessor table_accessor;
  bulgeTableAccessor_reset(&table_accessor, block_buffer);

  // find final block in directory, update it to point to our free block
  uint32_t block_current = directory_block;
  uint32_t block_current_value;
  while(true) {
    error = bulgeTableAccessor_getEntry(filesystem, &table_accessor, block_current, &block_current_value);
    if(error > BULGE_ERROR_NONE) { return error; } // read error?
    if(block_current_value == BULGE_TABLE_FINAL) { break; }
    if(block_current_value == BULGE_TABLE_FREE ) { return BULGE_ERROR_CORRUPTED_CHAIN; } // corruption
    block_current = block_current_value;
  }
  error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, block_current, block_free);
  if(error > BULGE_ERROR_NONE) { return error; }

  // claim our free block
  error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, block_free, BULGE_TABLE_FINAL);
  if(error > BULGE_ERROR_NONE) { return error; }

  return BULGE_ERROR_NONE;
}

BulgeError bulgeUtility_findEmptyEntry(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* block_buffer, BulgeEntry** entry_pointer) {
  BulgeEntryAccessor entry_accessor;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);

  uint32_t entry_index = 0;
  BulgeEntry* entry_current;

  while(true) {
    BulgeError error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, directory_block, entry_index, &entry_current);
    if(error == BULGE_ERROR_END_OF_DIRECTORY) {
      // this failure means our entry_index has overflowed the directory entry extents, try to allocate another block to it
      error = bulgeUtility_growDirectory(filesystem, directory_block, block_buffer);
      if(error > BULGE_ERROR_NONE) { return error; } // out of space?
      bulgeEntryAccessor_reset(&entry_accessor, block_buffer); // must reset since we've messed with our buffer
      continue; // re-run with our current index
    } else if(error > BULGE_ERROR_NONE) {
      return error;
    }
    ++entry_index;
    if(entry_current->type == BULGE_ENTRY_TYPE_FREE) { break; }
  }

  *entry_pointer = entry_current;
  return BULGE_ERROR_NONE;
}

BulgeError bulgeUtility_findNamedEntry(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* name_begin, uint8_t* name_end, uint8_t* block_buffer, BulgeEntry** entry_pointer) {
  BulgeEntryAccessor entry_accessor;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);

  uint32_t entry_index = 0;
  BulgeEntry* entry_current;

  while(true) {
    BulgeError error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, directory_block, entry_index, &entry_current);
    if(error == BULGE_ERROR_END_OF_DIRECTORY) { return BULGE_ERROR_PATH_NOT_FOUND; } // not found
    if(error > BULGE_ERROR_NONE) { return error; }
    ++entry_index;
    if(entry_current->type < BULGE_ENTRY_TYPE_FILE_LOWER     ) { continue; } // skip anything that's not a file or directory
    if(entry_current->type > BULGE_ENTRY_TYPE_DIRECTORY_UPPER) { continue; } // skip anything that's not a file or directory

    bool match = true;
    uint8_t* entry_name = entry_current->name;
    for(uint8_t* name=name_begin; name<=name_end; ++name) {
      if(entry_name[0] != name[0]) { match = false; break; }
      ++entry_name;
    }
    if(entry_name[0]) { continue; } // all of passed name matched, but entry name keeps going

    if(match) { break; }
  }

  *entry_pointer = entry_current;
  return BULGE_ERROR_NONE;
}

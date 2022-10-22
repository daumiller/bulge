#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "path.h"
#include "endian.h"
#include "entry-accessor.h"
#include "table-accessor.h"

typedef struct structPathSeparator {
  uint8_t* current_entry_begin;
  uint8_t* current_entry_end;
  bool     complete;
} PathSeparator;

static bool pathSeparator_reset(PathSeparator* separator, uint8_t* path) {
  // verify begins with '/'
  if(!path) { return false; }
  if(*path == 0  ) { return false; }
  if(*path != '/') { return false; }

  // verify doesn't end with '/', and doesn't have any "//"
  separator->current_entry_end = path;
  while(separator->current_entry_end[0]) {
    if((separator->current_entry_end[0] == '/') && (separator->current_entry_end[1] == '/')) { return false; }
    separator->current_entry_end += 1;
  }
  separator->current_entry_end -= 1;
  if(separator->current_entry_end[0] == '/') { return false; }

  // set beginning/ending point
  separator->current_entry_begin = path;
  separator->current_entry_end   = path;
  separator->complete            = false;
  return true;
}

static bool pathSeparator_nextComponent(PathSeparator* separator) {
  if(separator->complete) { return true; }

  // start at next character, after ending of last component
  separator->current_entry_begin = separator->current_entry_end + 1;
  // test if we're done
  if(separator->current_entry_begin[0] == 0) {
    separator->complete = true;
    return true;
  }
  // if not, advance past '/'
  if(separator->current_entry_begin[0] == '/') { separator->current_entry_begin += 1; }

  // find ending point
  separator->current_entry_end = separator->current_entry_begin + 1;
  while(true) {
    if(separator->current_entry_end[0] == '/') { break; }
    if(separator->current_entry_end[0] ==  0 ) { break; }
    separator->current_entry_end += 1;
  }
  separator->current_entry_end -= 1;

  return true;
}

// -----------------------------------------------------------------------------

static bool findFreeBlock(BulgeFilesystem* filesystem, uint32_t* block, uint8_t* block_buffer) {
  BulgeTableAccessor table_accessor;
  bulgeTableAccessor_reset(&table_accessor, block_buffer);

  uint32_t block_current = 0;
  uint32_t block_maximum = filesystem->blocks_total - 1;
  uint32_t block_current_value;
  while(block_current < block_maximum) {
    if(bulgeTableAccessor_getEntry(filesystem, &table_accessor, block_current, &block_current_value) == false) { return false; }
    if(block_current_value == BULGE_TABLE_FREE) {
      *block = block_current;
      return true;
    }
    ++block_current;
  }

  return false;
}

static bool growDirectory(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* block_buffer) {
  // get a free block
  uint32_t block_free;
  if(findFreeBlock(filesystem, &block_free, block_buffer) == false) { return false; }

  BulgeTableAccessor table_accessor;
  bulgeTableAccessor_reset(&table_accessor, block_buffer);

  // find final block in directory, update it to point to our free block
  uint32_t block_current = directory_block;
  uint32_t block_current_value;
  while(true) {
    if(bulgeTableAccessor_getEntry(filesystem, &table_accessor, block_current, &block_current_value) == false) { return false; } // must be a read error
    if(block_current_value == BULGE_TABLE_FINAL) { break; }
    if(block_current_value == BULGE_TABLE_FREE ) { return false; } // corruption
    block_current = block_current_value;
  }
  bulgeTableAccessor_setEntry(filesystem, &table_accessor, block_current, block_free);

  // claim our free block
  bulgeTableAccessor_setEntry(filesystem, &table_accessor, block_free, BULGE_TABLE_FINAL);

  return true;
}

static bool findEmptyEntry(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* block_buffer, BulgeEntry** entry_pointer) {
  BulgeEntryAccessor entry_accessor;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);

  uint32_t entry_index = 0;
  BulgeEntry* entry_current;

  while(true) {
    if(bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, directory_block, entry_index, &entry_current) == false) {
      // TODO: we REALLY need return status codes here
      // for now, lets assume this failure means our entry_index has overflowed the directory entry extents
      // and try to allocate another block to it
      if(growDirectory(filesystem, directory_block, block_buffer) == false) { return false; } // must be out of space
      bulgeEntryAccessor_reset(&entry_accessor, block_buffer); // must reset since we've messed with our buffer
      continue; // re-run with our current index
    }
    ++entry_index;
    if(entry_current->type == BULGE_ENTRY_TYPE_FREE) { break; }
  }

  *entry_pointer = entry_current;
  return true;
}

static bool findNamedEntry(BulgeFilesystem* filesystem, uint8_t directory_block, uint8_t* name_begin, uint8_t* name_end, uint8_t* block_buffer, BulgeEntry** entry_pointer) {
  BulgeEntryAccessor entry_accessor;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);

  uint32_t entry_index = 0;
  BulgeEntry* entry_current;

  while(true) {
    if(bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, directory_block, entry_index, &entry_current) == false) { return false; }
    ++entry_index;
    if(entry_current->type == BULGE_ENTRY_TYPE_END) { return false; } // not found
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
  return true;
}

bool bulgePath_findEntry(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, BulgeEntry** entry_pointer) {
  // special case for root directory
  if((path == 0) || (*path == 0)) {
    BulgeEntryAccessor entry_accessor;
    bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
    if(bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, filesystem->block_root, 0, entry_pointer) == false) { return false; }
    return true;
  }

  PathSeparator separator;
  if(pathSeparator_reset(&separator, path) == false) { return false; } // invalid path

  uint32_t block_directory_current = filesystem->block_root;
  BulgeEntry* entry_current;
  bool match_is_directory = true;

  while(true) {
    if(pathSeparator_nextComponent(&separator) == false) { return false; }
    if(separator.complete) { break; }
    if(!match_is_directory) { return false; } // we have more components to search for, but last match encountered was not a directory

    if(!findNamedEntry(filesystem, block_directory_current, separator.current_entry_begin, separator.current_entry_end, block_buffer, &entry_current)) { return false; }

    if((entry_current->type >= BULGE_ENTRY_TYPE_DIRECTORY_UPPER) && (entry_current->type <= BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
      block_directory_current = BULGE_ENDIAN32(entry_current->block);
      match_is_directory = true;
    } else {
      match_is_directory = false;
    }
  }

  // return our result!
  *entry_pointer = entry_current;
  return true;
}

bool bulgePath_openDirectory(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_entry_block) {
  PathSeparator separator;
  if(pathSeparator_reset(&separator, path) == false) {
    printf("  Invalid Path!\n");
    return false;
  }
  while(true) {
    if(pathSeparator_nextComponent(&separator) == false) {
      printf("  Invalid Path!\n");
      return false;
    }
    if(separator.complete) { return true; }

    printf("  Path component: ");
    for(uint8_t* component = separator.current_entry_begin; component <= separator.current_entry_end; ++component) {
      printf("%c", *component);
    }
    printf("\n");
  }

  return true;
}

bool bulgePath_openFile(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_data_block) {
  return false;
}

bool bulgePath_openDirectoryAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_attribute_block) {
  return false;
}

bool bulgePath_openFileAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_attribute_block) {
  return false;
}

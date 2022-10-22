#include <bulge/bulge.h>
#include "path-separator.h"
#include "utility.h"

BulgeError bulgePath_findEntry(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, BulgeEntry** entry_pointer) {
  BulgeError error;

  // special case for root directory
  if((path == 0) || (*path == 0)) {
    BulgeEntryAccessor entry_accessor;
    bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
    error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, filesystem->block_root, 0, entry_pointer);
    if(error > BULGE_ERROR_NONE) { return error; }
    return BULGE_ERROR_NONE;
  }

  BulgePathSeparator separator;
  if(bulgePathSeparator_reset(&separator, path) == false) { return BULGE_ERROR_PATH_INVALID; }

  uint32_t block_directory_current = filesystem->block_root;
  BulgeEntry* entry_current;
  bool match_is_directory = true;

  while(true) {
    if(bulgePathSeparator_nextComponent(&separator) == false) { return BULGE_ERROR_PATH_INVALID; }
    if(separator.complete) { break; }
    if(!match_is_directory) { return BULGE_ERROR_PATH_NOT_FOUND; } // we have more components to search for, but last match encountered was not a directory

    error = bulgeUtility_findNamedEntry(filesystem, block_directory_current, separator.current_entry_begin, separator.current_entry_end, block_buffer, &entry_current);
    if(error > BULGE_ERROR_NONE) { return error; }

    if((entry_current->type >= BULGE_ENTRY_TYPE_DIRECTORY_UPPER) && (entry_current->type <= BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
      block_directory_current = BULGE_ENDIAN32(entry_current->block);
      match_is_directory = true;
    } else {
      match_is_directory = false;
    }
  }

  // return our result!
  *entry_pointer = entry_current;
  return BULGE_ERROR_NONE;
}

BulgeError bulgePath_openDirectory(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_entry_block) {
  // BulgePathSeparator separator;
  // if(bulgePathSeparator_reset(&separator, path) == false) {
  //   printf("  Invalid Path!\n");
  //   return false;
  // }
  // while(true) {
  //   if(bulgePathSeparator_nextComponent(&separator) == false) {
  //     printf("  Invalid Path!\n");
  //     return false;
  //   }
  //   if(separator.complete) { return true; }

  //   printf("  Path component: ");
  //   for(uint8_t* component = separator.current_entry_begin; component <= separator.current_entry_end; ++component) {
  //     printf("%c", *component);
  //   }
  //   printf("\n");
  // }

  return BULGE_ERROR_NOT_IMPLEMENTED;
}

BulgeError bulgePath_openFile(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_data_block) {
  return BULGE_ERROR_NOT_IMPLEMENTED;
}

BulgeError bulgePath_openDirectoryAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_attribute_block) {
  return BULGE_ERROR_NOT_IMPLEMENTED;
}

BulgeError bulgePath_openFileAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_attribute_block) {
  return BULGE_ERROR_NOT_IMPLEMENTED;
}

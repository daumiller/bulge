#include <bulge/bulge.h>
#include "path-separator.h"
#include "utility.h"

BulgeError bulgePath_findParent(BulgeFilesystem* filesystem, uint8_t* path, uint32_t* parent_block, uint8_t* block_buffer, uint8_t** name_begins) {
  uint32_t last_slash = UINT32_MAX;
  uint8_t* path_end = path;
  while(path_end[0]) {
    if(path_end[0] == '/') { last_slash = path_end - path; }
    ++path_end;
  }
  if(last_slash == UINT32_MAX) { return BULGE_ERROR_PATH_INVALID; }
  path_end = path + last_slash;
  if(name_begins) { *name_begins = path_end + 1; }

  BulgeEntry* entry;
  {
    uint32_t length = path_end - path;
    uint8_t path_temp[length];
    for(uint32_t idx=0; idx<length; ++idx) { path_temp[idx] = path[idx]; }
    path_temp[length] = 0;
    BulgeError error = bulgePath_findEntry(filesystem, path_temp, block_buffer, &entry, 0, 0);
    if(error > BULGE_ERROR_NONE) { return error; }
  }

  if(entry->type != BULGE_ENTRY_TYPE_SELF) { // if not root directory, ensure some kind of directory
    if((entry->type < BULGE_ENTRY_TYPE_DIRECTORY_LOWER) || (entry->type > BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
      return BULGE_ERROR_PATH_INVALID;
    }
  }

  *parent_block = BULGE_ENDIAN32(entry->block);
  return BULGE_ERROR_NONE;
}

BulgeError bulgePath_findEntry(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, BulgeEntry** entry_pointer, uint32_t* parent_block, uint32_t* parent_index) {
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
  uint32_t entry_index = 0;

  while(true) {
    if(bulgePathSeparator_nextComponent(&separator) == false) { return BULGE_ERROR_PATH_INVALID; }
    if(separator.complete) { break; }
    if(!match_is_directory) { return BULGE_ERROR_PATH_NOT_FOUND; } // we have more components to search for, but last match encountered was not a directory

    error = bulgeUtility_findNamedEntry(filesystem, block_directory_current, separator.current_entry_begin, separator.current_entry_end, block_buffer, &entry_current, &entry_index);
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
  if(parent_block) { *parent_block = block_directory_current; }
  if(parent_index) { *parent_index = entry_index; }
  return BULGE_ERROR_NONE;
}

BulgeError bulgePath_modifyEntry(BulgeFilesystem* filesystem, BulgeEntry* modified_entry, uint32_t parent_block, uint32_t parent_index, uint8_t* block_buffer) {
  // first, ensure the new name is valid
  uint8_t* modified_name_end = modified_entry->name;
  while(modified_name_end[0]) { ++modified_name_end; }
  modified_name_end -= 1;
  if(modified_name_end == modified_entry->name) { return BULGE_ERROR_PATH_INVALID; }

  // now, in case we're renaming the file, make sure something with the given name doesn't exist anywhere else
  BulgeEntry* entry;
  uint32_t name_match_index;
  BulgeError error = bulgeUtility_findNamedEntry(filesystem, parent_block, modified_entry->name, modified_name_end, block_buffer, &entry, &name_match_index);
  if(error == BULGE_ERROR_NONE) {
    // if this name already exists at this location, ensure it belongs to the current entry
    if(name_match_index != parent_index) {
      return BULGE_ERROR_PATH_EXISTS;
    }
  }

  // get the existing entry
  BulgeEntryAccessor entry_accessor;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
  error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, parent_block, parent_index, &entry);
  if(error > BULGE_ERROR_NONE) { return error; }

  // if we're updating the type, make sure its within the same type class

  if(modified_entry->type != entry->type) {
    if((entry->type >= BULGE_ENTRY_TYPE_DIRECTORY_LOWER) && (entry->type <= BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
      if((modified_entry->type < BULGE_ENTRY_TYPE_DIRECTORY_LOWER) || (modified_entry->type > BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
        return BULGE_ERROR_ENTRY_TYPE_MISMATCH;
      }
    } else if((entry->type >= BULGE_ENTRY_TYPE_FILE_LOWER) && (entry->type <= BULGE_ENTRY_TYPE_FILE_UPPER)) {
      if((modified_entry->type < BULGE_ENTRY_TYPE_FILE_LOWER) || (modified_entry->type > BULGE_ENTRY_TYPE_FILE_UPPER)) {
        return BULGE_ERROR_ENTRY_TYPE_MISMATCH;
      }
    } else if(entry->type != BULGE_ENTRY_TYPE_FREE) {
      // allow chaning from FREE, to allocate new entries
      // otherwise, shouldn't be changed
      return BULGE_ERROR_ENTRY_TYPE_MISMATCH;
    }
  }

  // copy over valid-to-update fields
  entry->type             = modified_entry->type;
  entry->user             = modified_entry->user;
  entry->group            = modified_entry->user;
  entry->permissions      = modified_entry->permissions;
  entry->time_created[0]  = modified_entry->time_created[0];
  entry->time_created[1]  = modified_entry->time_created[1];
  entry->time_modified[0] = modified_entry->time_modified[0];
  entry->time_modified[1] = modified_entry->time_modified[1];
  entry->system_flags     = modified_entry->system_flags;
  for(uint8_t idx=0; idx<80; ++idx) {
    entry->name[idx] = modified_entry->name[idx];
  }

  bulgeEntryAccessor_setDirty(&entry_accessor);
  return bulgeEntryAccessor_flush(filesystem, &entry_accessor);
}

static BulgeError bulgePath_createType(BulgeFilesystem* filesystem, uint8_t* path, BulgeEntry* entry, uint8_t* block_buffer) {
  // first, find parent directory
  uint32_t parent_block = 0;
  uint8_t* name_begins;
  BulgeError error = bulgePath_findParent(filesystem, path, &parent_block, block_buffer, &name_begins);
  if(error > BULGE_ERROR_NONE) { return error; }

  // check new directory name is valid
  uint8_t* name_ends = name_begins;
  while(name_ends[0]) { ++name_ends; }
  name_ends -= 1;
  if(name_ends == name_begins) { return BULGE_ERROR_PATH_INVALID; }

  // ensure an item with this name doesn't already exist
  BulgeEntry* new_entry;
  error = bulgeUtility_findNamedEntry(filesystem, parent_block, name_begins, name_ends, block_buffer, &new_entry, 0);
  if(error == BULGE_ERROR_NONE) { return BULGE_ERROR_PATH_EXISTS; }

  // find a free block
  uint32_t free_block;
  error = bulgeUtility_findFreeBlock(filesystem, &free_block, block_buffer);
  if(error > BULGE_ERROR_NONE) { return error; }

  // attempt to claim block
  BulgeTableAccessor table_accessor;
  bulgeTableAccessor_reset(&table_accessor, block_buffer);
  error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, free_block, BULGE_TABLE_FINAL);
  if(error > BULGE_ERROR_NONE) { return error; }
  error = bulgeTableAccessor_flush(filesystem, &table_accessor);
  if(error > BULGE_ERROR_NONE) { return error; }
  filesystem->blocks_free--;
  filesystem->blocks_occupied++;

  // find a free entry (in parent)
  uint32_t free_entry_index;
  error = bulgeUtility_findEmptyEntry(filesystem, parent_block, block_buffer, &new_entry, &free_entry_index);
  if(error > BULGE_ERROR_NONE) {
    // unclaim free block
    bulgeTableAccessor_reset(&table_accessor, block_buffer);
    BulgeError secondary_error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, free_entry_index, BULGE_TABLE_FREE);
    if(secondary_error == BULGE_ERROR_NONE) {
      secondary_error = bulgeTableAccessor_flush(filesystem, &table_accessor);
      if(secondary_error == BULGE_ERROR_NONE) {
        filesystem->blocks_free++;
        filesystem->blocks_occupied--;
      }
    }
    return error;
  }

  // attempt to update entry (in parent)
  BulgeEntryAccessor entry_accessor;
  bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
  error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, parent_block, free_entry_index, &new_entry);
  if(error > BULGE_ERROR_NONE) {
    // unclaim free block
    bulgeTableAccessor_reset(&table_accessor, block_buffer);
    BulgeError secondary_error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, free_entry_index, BULGE_TABLE_FREE);
    if(secondary_error == BULGE_ERROR_NONE) {
      secondary_error = bulgeTableAccessor_flush(filesystem, &table_accessor);
      if(secondary_error == BULGE_ERROR_NONE) {
        filesystem->blocks_free++;
        filesystem->blocks_occupied--;
      }
    }
    return error;
  }

  new_entry->type             = entry->type;
  new_entry->user             = entry->user;
  new_entry->group            = entry->group;
  new_entry->permissions      = entry->permissions;
  new_entry->block            = BULGE_ENDIAN32(free_block);
  new_entry->size_bytes       = 0;
  new_entry->size_blocks      = BULGE_ENDIAN32(1);
  new_entry->time_created[0]  = 0;
  new_entry->time_created[1]  = 0;
  new_entry->time_modified[0] = 0;
  new_entry->time_modified[1] = 0;
  new_entry->attribute_block  = 0;
  new_entry->attribute_size   = 0;
  new_entry->system_flags     = BULGE_ENDIAN32(entry->system_flags);
  new_entry->reserved         = 0;
  name_ends = name_begins;
  for(uint8_t idx=0; idx<79; ++idx) {
    if(name_ends[0]) {
      new_entry->name[idx] = name_ends[0];
      ++name_ends;
    } else {
      new_entry->name[idx] = 0;
    }
  }
  new_entry->name[79] = 0;

  bulgeEntryAccessor_setDirty(&entry_accessor);
  error = bulgeEntryAccessor_flush(filesystem, &entry_accessor);
  if(error > BULGE_ERROR_NONE) {
    // unclaim free block
    bulgeTableAccessor_reset(&table_accessor, block_buffer);
    BulgeError secondary_error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, free_block, BULGE_TABLE_FREE);
    if(secondary_error == BULGE_ERROR_NONE) {
      secondary_error = bulgeTableAccessor_flush(filesystem, &table_accessor);
      if(secondary_error == BULGE_ERROR_NONE) {
        filesystem->blocks_free++;
        filesystem->blocks_occupied--;
      }
    }
    return error;
  }

  // if a directory
  if((entry->type >= BULGE_ENTRY_TYPE_DIRECTORY_LOWER) && (entry->type <= BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
    // attempt to create self entry
    bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
    error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, free_block, 0, &new_entry);
    if(error > BULGE_ERROR_NONE) {
      // remove record from parent
      bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
      BulgeError secondary_error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, parent_block, free_entry_index, &new_entry);
      if(error == BULGE_ERROR_NONE) {
        new_entry->type = BULGE_ENTRY_TYPE_FREE;
        bulgeEntryAccessor_setDirty(&entry_accessor);
        bulgeEntryAccessor_flush(filesystem, &entry_accessor);
      }
      // unclaim free block
      bulgeTableAccessor_reset(&table_accessor, block_buffer);
      secondary_error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, free_block, BULGE_TABLE_FREE);
      if(secondary_error == BULGE_ERROR_NONE) {
        secondary_error = bulgeTableAccessor_flush(filesystem, &table_accessor);
        if(secondary_error == BULGE_ERROR_NONE) {
          filesystem->blocks_free++;
          filesystem->blocks_occupied--;
        }
      }
      return error;
    }

    BulgeEntrySelf* self_entry = (BulgeEntrySelf*)new_entry;
    self_entry->type = BULGE_ENTRY_TYPE_SELF;
    self_entry->root_user             = 0;
    self_entry->root_group            = 0;
    self_entry->root_permissions      = 0;
    self_entry->block_self            = BULGE_ENDIAN32(free_block);
    self_entry->block_parent          = BULGE_ENDIAN32(parent_block);
    self_entry->root_size_blocks      = 0;
    self_entry->root_time_created[0]  = 0;
    self_entry->root_time_created[1]  = 0;
    self_entry->root_time_modified[0] = 0;
    self_entry->root_time_modified[1] = 0;
    self_entry->root_attribute_block  = 0;
    self_entry->root_attribute_size   = 0;
    self_entry->root_system_flags     = 0;
    self_entry->reserved              = 0;
    name_ends = name_begins;
    for(uint8_t idx=0; idx<79; ++idx) {
      if(name_ends[0]) {
        self_entry->name[idx] = name_ends[0];
        ++name_ends;
      } else {
        self_entry->name[idx] = 0;
      }
    }
    bulgeEntryAccessor_setDirty(&entry_accessor);
    // attempt to create free entries in self
    for(uint32_t idx=1; idx<4; ++idx) {
      bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, free_block, idx, &new_entry);
      new_entry->type = BULGE_ENTRY_TYPE_FREE;
      bulgeEntryAccessor_setDirty(&entry_accessor);
    }
    error = bulgeEntryAccessor_flush(filesystem, &entry_accessor);
    if(error > BULGE_ERROR_NONE) {
      // remove record from parent
      bulgeEntryAccessor_reset(&entry_accessor, block_buffer);
      BulgeError secondary_error = bulgeEntryAccessor_getEntry(filesystem, &entry_accessor, parent_block, free_entry_index, &new_entry);
      if(error == BULGE_ERROR_NONE) {
        new_entry->type = BULGE_ENTRY_TYPE_FREE;
        bulgeEntryAccessor_setDirty(&entry_accessor);
        bulgeEntryAccessor_flush(filesystem, &entry_accessor);
      }
      // unclaim free block
      bulgeTableAccessor_reset(&table_accessor, block_buffer);
      secondary_error = bulgeTableAccessor_setEntry(filesystem, &table_accessor, free_block, BULGE_TABLE_FREE);
      if(secondary_error == BULGE_ERROR_NONE) {
        secondary_error = bulgeTableAccessor_flush(filesystem, &table_accessor);
        if(secondary_error == BULGE_ERROR_NONE) {
          filesystem->blocks_free++;
          filesystem->blocks_occupied--;
        }
      }
      return error;
    }
  }

  // success!
  return BULGE_ERROR_NONE;
}

BulgeError bulgePath_createDirectory(BulgeFilesystem* filesystem, uint8_t* path, BulgeEntry* entry, uint8_t* block_buffer) {
  // ensure type is valid
  if((entry->type < BULGE_ENTRY_TYPE_DIRECTORY_LOWER) || (entry->type > BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
    return BULGE_ERROR_ENTRY_TYPE_MISMATCH;
  }

  return bulgePath_createType(filesystem, path, entry, block_buffer);
}

BulgeError bulgePath_createFile(BulgeFilesystem* filesystem, uint8_t* path, BulgeEntry* entry, uint8_t* block_buffer) {
  // ensure type is valid
  if((entry->type < BULGE_ENTRY_TYPE_FILE_LOWER) || (entry->type > BULGE_ENTRY_TYPE_FILE_UPPER)) {
    return BULGE_ERROR_ENTRY_TYPE_MISMATCH;
  }

  return bulgePath_createType(filesystem, path, entry, block_buffer);
}

BulgeError bulgePath_openDirectory(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* directory_entry_block) {
  BulgeEntry* entry_pointer;
  BulgeError error = bulgePath_findEntry(filesystem, path, block_buffer, &entry_pointer, 0, 0);
  if(error > BULGE_ERROR_NONE) { return error; }
  if((entry_pointer->type < BULGE_ENTRY_TYPE_DIRECTORY_LOWER) || (entry_pointer->type > BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
    return BULGE_ERROR_PATH_NOT_A_DIRECTORY;
  }

  // in case of root directory, this will be a BulgeEntrySelf, but the "block_self" entry maps to "block" on BulgeEntry
  *directory_entry_block = BULGE_ENDIAN32(entry_pointer->block);
  return BULGE_ERROR_NONE;
}

BulgeError bulgePath_openFile(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* file_data_block) {
  BulgeEntry* entry_pointer;
  BulgeError error = bulgePath_findEntry(filesystem, path, block_buffer, &entry_pointer, 0, 0);
  if(error > BULGE_ERROR_NONE) { return error; }
  if((entry_pointer->type < BULGE_ENTRY_TYPE_FILE_LOWER) || (entry_pointer->type > BULGE_ENTRY_TYPE_FILE_UPPER)) {
    return BULGE_ERROR_PATH_NOT_A_FILE;
  }

  *file_data_block = BULGE_ENDIAN32(entry_pointer->block);
  return BULGE_ERROR_NONE;
}

BulgeError bulgePath_openAttributes(BulgeFilesystem* filesystem, uint8_t* path, uint8_t* block_buffer, uint32_t* attribute_block) {
  BulgeEntry* entry_pointer;
  BulgeError error = bulgePath_findEntry(filesystem, path, block_buffer, &entry_pointer, 0, 0);
  if(error > BULGE_ERROR_NONE) { return error; }
  if((entry_pointer->type < BULGE_ENTRY_TYPE_DIRECTORY_LOWER) || (entry_pointer->type > BULGE_ENTRY_TYPE_DIRECTORY_UPPER)) {
    return BULGE_ERROR_PATH_NOT_A_DIRECTORY;
  }

  // in case of root directory, this will be a BulgeEntrySelf, but the "root_attributes_block" entry maps to "attributes_block" on BulgeEntry
  if(entry_pointer->attribute_block == BULGE_TABLE_FREE) {
    return BULGE_ERROR_ATTRIBUTES_NOT_CREATED;
  }

  *attribute_block = entry_pointer->attribute_block;
  return BULGE_ERROR_NONE;
}

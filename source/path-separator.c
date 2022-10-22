#include "path-separator.h"

bool bulgePathSeparator_reset(BulgePathSeparator* separator, uint8_t* path) {
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

bool bulgePathSeparator_nextComponent(BulgePathSeparator* separator) {
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

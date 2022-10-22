#include <bulge/bulge.h>

const char* bulgeError_string(BulgeError error) {
#ifdef BULGE_ERROR_STRINGS
  return "(compiled without error strings)";
#endif // ifdef BULGE_ERROR_STRINGS

  switch(error) {
    case BULGE_ERROR_NONE                 : return "no error";
    case BULGE_ERROR_DEVICE_READ          : return "system-provided device read function failed";
    case BULGE_ERROR_DEVICE_WRITE         : return "system-provided device write function failed";
    case BULGE_ERROR_FULL_FILESYSTEM      : return "filesystem is full";
    case BULGE_ERROR_FULL_DIRECTORY       : return "directory is full";
    case BULGE_ERROR_PATH_INVALID         : return "given path is invalid";
    case BULGE_ERROR_PATH_NOT_FOUND       : return "given path was not found";
    case BULGE_ERROR_CORRUPTED_FILESYSTEM : return "filesystem corruption";
    case BULGE_ERROR_CORRUPTED_TABLE      : return "chain table corrupted";
    case BULGE_ERROR_CORRUPTED_CHAIN      : return "data chain corrupted";
    case BULGE_ERROR_CORRUPTED_ENTRY      : return "directory entry corrupted";
    case BULGE_ERROR_CORRUPTED_DIRECTORY  : return "directory data corrupted";
    case BULGE_ERROR_END_OF_DIRECTORY     : return "end of directory passed";
    case BULGE_ERROR_TOO_SMALL            : return "cannot create a filesystem that small";
    case BULGE_ERROR_BUG                  : return "implementation bug (you shouldn't see this error)";
    case BULGE_ERROR_NOT_BULGE_FS         : return "not a bulge filesystem";
    case BULGE_ERROR_FS_VERSION_TOO_NEW   : return "filesystem version is newer than this implementation";
    case BULGE_ERROR_NOT_IMPLEMENTED      : return "function not yet implemented (you shouldn't see this";
  }

  return "(unknown error)";
}

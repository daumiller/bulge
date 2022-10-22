#ifndef BULGE_STORAGE_TYPES_HEADER
#define BULGE_STORAGE_TYPES_HEADER

#include <stdint.h>
#include "endian.h"

#define BULGE_TABLE_FREE  0x00000000
#define BULGE_TABLE_FINAL 0x00000001

typedef struct __attribute__((__packed__)) structBulgeHeader {
  uint8_t  signature[8];      /// "BulgeFS!"
  uint32_t version;           /// version this filesystem was created with
  uint32_t block_count_total; /// total number of blocks in filesystem
  uint32_t block_count_table; /// number of blocks used for chain table
  uint32_t blocks_occupied;   /// number of blocks used
  uint32_t blocks_free;       /// number of blocks unused
  uint32_t root_block;        /// first block of root directory
  uint8_t  reerved[48];       /// reserved for future use
  uint8_t  system[32];        /// system-specific data
  uint8_t  uuid[16];          /// filesystem identifier
  uint8_t  name[128];         /// filesystem name
} BulgeHeader; // 256 bytes

#define BULGE_ENTRY_TYPE_FREE            0x00
#define BULGE_ENTRY_TYPE_SELF            0x01
#define BULGE_ENTRY_TYPE_FILE_LOWER      0x20
#define BULGE_ENTRY_TYPE_FILE_UPPER      0x5F
#define BULGE_ENTRY_TYPE_DIRECTORY_LOWER 0x60
#define BULGE_ENTRY_TYPE_DIRECTORY_UPPER 0x9F
#define BULGE_ENTRY_TYPE_END             0xFF
typedef uint8_t BulgeEntryType;

typedef struct __attribute__((__packed__)) structBulgeEntry {
  BulgeEntryType type;             /// type of entry
  uint8_t        user;             /// user id
  uint8_t        group;            /// group id
  uint8_t        permissions;      /// xsrwrwrw (executable, set-user-id, other:rw, group:rw, user:rw)
  uint32_t       block;            /// starting block of main data
  uint32_t       size_bytes;       /// for files, size of main data in bytes
  uint32_t       size_blocks;      /// size of main data in blocks
  uint32_t       time_created[2];  /// creation time
  uint32_t       time_modified[2]; /// modification time
  uint32_t       attribute_block;  /// starting block of attribute data
  uint32_t       attribute_size;   /// size of attribute data in blocks
  uint32_t       system_flags;     /// system-specific flags/data
  uint32_t       reserved;         /// reserved for future use
  uint8_t        name[80];         /// name
} BulgeEntry; // 128 bytes

typedef struct __attribute__((__packed__)) structBulgeEntrySelf {
  BulgeEntryType type;                  /// type of entry: BULGE_ENTRY_TYPE_SELF
  uint8_t        root_user;             /// only for root directory: user id
  uint8_t        root_group;            /// only for root directory: group id
  uint8_t        root_permissions;      /// only for root directory: xsrwrwrw (executable, set-user-id, other:rw, group:rw, user:rw)
  uint32_t       block_self;            /// starting block of self (used for identifying self in parent table)
  uint32_t       block_parent;          /// starting block of parent directory
  uint32_t       root_size_blocks;      /// only for root directory: size of main data in blocks
  uint32_t       root_time_created[2];  /// only for root directory: creation time
  uint32_t       root_time_modified[2]; /// only for root directory: modification time
  uint32_t       root_attribute_block;  /// only for root directory: starting block of attribute data
  uint32_t       root_attribute_size;   /// only for root directory: size of attribute data in blocks
  uint32_t       root_system_flags;     /// system-specific flags/data
  uint32_t       reserved;              /// reserved for future use
  uint8_t        name[80];              /// name
} BulgeEntrySelf; // 128 bytes

#endif // ifndef BULGE_STORAGE_TYPES_HEADER

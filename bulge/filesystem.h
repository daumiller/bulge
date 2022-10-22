#ifndef BULGE_FILESYSTEM_HEADER
#define BULGE_FILESYSTEM_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <bulge/error.h>

typedef bool (*bulgeBlockRead)(uint32_t block, uint8_t* buffer, void* callback_data);
typedef bool (*bulgeBlockWrite)(uint32_t block, uint8_t* buffer, void* callback_data);

typedef struct structBulgeFilesystem {
  uint32_t        block_filesystem; /// block address of filesystem
  uint32_t        block_table;      /// block address of chain table
  uint32_t        block_root;       /// block address of root directory
  uint32_t        blocks_total;     /// total filesystem blocks
  uint32_t        blocks_occupied;  /// used filesystem blocks
  uint32_t        blocks_free;      /// unused filesystem blocks
  uint8_t         system[32];       /// system-specific filesystem data
  bulgeBlockRead  read;             /// system-provided block reading function
  bulgeBlockWrite write;            /// system-provided block writing function
  void*           callback_data;    /// system-provided data to pass to read/write functions
} BulgeFilesystem; // 68 Bytes (for 32 bit systems)

typedef struct structBulgeFilesystemInformation {
  uint32_t version;      /// version filesystem was created with
  uint32_t blocks_total; /// filesystem size, in blocks
  uint32_t blocks_free;  /// total free blocks available
  uint8_t  system[32];   /// system-specific data
  uint8_t  uuid[16];     /// filesystem id
  uint8_t  name[128];    /// name
} BulgeFilesystemInformation; // 188 Bytes

BulgeError bulgeFilesystem_create(BulgeFilesystem* filesystem, uint32_t block_begin, uint32_t block_count, bulgeBlockRead read, bulgeBlockWrite write, void* callback_data, uint8_t* block_buffer);
BulgeError bulgeFilesystem_open(BulgeFilesystem* filesystem, uint32_t block, bulgeBlockRead read, bulgeBlockWrite write, void* callback_data, uint8_t* block_buffer);
BulgeError bulgeFilesystem_close(BulgeFilesystem* filesystem, uint8_t* block_buffer);
BulgeError bulgeFilesystem_getInformation(BulgeFilesystem* filesystem, BulgeFilesystemInformation* information, uint8_t* block_buffer);
BulgeError bulgeFilesystem_setId(BulgeFilesystem* filesystem, uint8_t* uuid, uint8_t* block_buffer);
BulgeError bulgeFilesystem_setName(BulgeFilesystem* filesystem, uint8_t* name, uint8_t* block_buffer);
BulgeError bulgeFilesystem_setSystemData(BulgeFilesystem* filesystem, uint8_t* system, uint8_t* block_buffer);

#endif // ifndef BULGE_FILESYSTEM_HEADER

The Bulge Filesystem
================================================================================

  **A simple, flexible, FAT-like, filesystem. Targeted at modest-resource 32 bit systems.**

  - fixed sector/block size of 512 bytes
  - maximum filesystem size of 2 TiB
  - maximum file size of 4 GiB
  - maximum (unassisted) addressable range within a storage device of 2 TiB
    - if needed, systems may offset this during read/write functions with the system_data parameter
  - 80 byte file/directory name limit
    - zero-terminated, UTF8 or ASCII
  - reserved space for system-defined functionality
    - 32 reserved bytes in filesystem header
    -  4 byte system data per file (can point to other storage)
    - 64 types of each, files and directories, for system-defined differentation
    - supports extended attribute data for files/directories
  - support for users/groups/permissions
    - up to the system to enforce; default implementation does not
  - designed to not require memory allocation
    - some functions may be heavy on stack use
    - most functions will require system-provided buffers for I/O
  - designed for, but not limited to, 68k systems

  **Specifics for 0.1.0.0 default implementation**

  - not yet thread safe
  - not yet concurrent-action safe
    - *for example*: opening a file, deleting that file, creating a new file/directory, then writing to the original (deleted) file -- could cause corruption
    - future versions will provide either a better handle-management system internally, or callbacks to enable systems to better manage it externally

Layout Overview
--------------------------------------------------------------------------------

  | Location                                | Description                    |
  |-----------------------------------------|--------------------------------|
  | Block 0                                 | Filesystem Header              |
  | Block 1                                 | Beginning of Block Chain Table |
  | Immediately following Block Chain Table | Beginning of Root Directory    |

Filesystem Header
--------------------------------------------------------------------------------

  `Version 0.1.0.0`

  | offset | size | data type    | name                | description                                         |
  |--------|------|--------------|---------------------|-----------------------------------------------------|
  |    0   |   8  | uint8_t[8]   | signature           | "BulgeFS!" (NOT zero terminated)                    |
  |    8   |   4  | uint32_t     | version             | Version this fs was made with (x.y.z.w)             |
  |   12   |   4  | uint32_t     | block_count_total   | total number of blocks in this filesystem           |
  |   16   |   4  | uint32_t     | block_count_table   | total number of blocks used by the chain table      |
  |   20   |   4  | uint32_t     | blocks_occupied     | total number of used blocks                         |
  |   24   |   4  | uint32_t     | blocks_free         | total number of unused blocks                       |
  |   28   |   4  | uint32_t     | root_block          | first block number of root directory                |
  |   32   |  48  |              | (reserved)          | (reserved for future use)                           |
  |   80   |  32  |              | system-defined      | system-specific data (not defined or used def imp)  |
  |  112   |  16  | uint8_t[16]  | uuid                | filesystem identifier                               |
  |  128   | 128  | uint8_t[128] | name                | filesystem name                                     |
  |--------|------|--------------|---------------------|-----------------------------------------------------|
  |        | 256  |              |                     | **Header Total**                                    |
  |        | 256  |              |                     | **Available/Unused**                                |

Block Chain Table
--------------------------------------------------------------------------------
  - one uint32_t per block
  - value stored at this location gives the block number of the next block in this chain
    - `0x0000_00000` -> this block is free
    - `0x0000_00001` -> this block is used, and no further blocks in this chain
    - `0x0000_00002`-`0xFFFF_FFFF` -> block number of next block in this chain

Directories
--------------------------------------------------------------------------------
  - a list of 128 Byte entries
    - first entry is a directory's self data
    - subsequent entries describe child files/directories
    - a final entry is an end-of-directory marker
  - with only 4 entries per block, default implementation will pre-allocate extra blocks when creating directories

  `directory_entry_type` (*typedef uint8_t*)

  |  value(s)   |     description     |
  |-------------|---------------------|
  |      `0x00` | free entry          |
  |      `0x01` | directory self data |
  | `0x02-0x1F` | (reserved)          |
  | `0x20-0x5F` | child directory*    |
  | `0x60-0x9F` | child file*         |
  | `0xA0-0xFE` | (reserved)          |
  |      `0xFF` | end of directory    |

  * *64 different types may be used to differentiate system-specific file/directory purposes; all 64 will be treated the same by default implementation*

  **Child Directory Entry**

  | offset | size | data type            | name              | description                                                                     |
  |--------|------|----------------------|-------------------|---------------------------------------------------------------------------------|
  |    0   |   1  | directory_entry_type | type              | type of entry this is                                                           |
  |    1   |   1  | uint8_t              | user              | user id                                                                         |
  |    2   |   1  | uint8_t              | group             | group id                                                                        |
  |    3   |   1  | uint8_t              | permissions       | xsrwrwrw (executable, set-user-id, other:r/w, group:r/w, user:r/w )             |
  |    4   |   4  | uint32_t             | block             | starting block of file or directory main data                                   |
  |    8   |   4  | uint32_t             | size_bytes        | for files, size in bytes; for directories, unused                               |
  |   12   |   4  | uint32_t             | size_blocks       | size in blocks                                                                  |
  |   16   |   8  | uint64_t             | time_created      | created time (64 bit signed milliseconds since epoch ("Java-time"))             |
  |   24   |   8  | uint64_t             | time_modified     | modified time (64 bit signed milliseconds since epoch ("Java-time"))            |
  |   32   |   4  | uint32_t             | attribute_block   | starting block of file extended attributes                                      |
  |   36   |   4  | uint32_t             | attribute_size    | size, in blocks, of extended attributes                                         |
  |   40   |   4  | uint32_t             | system_flags      | system-specific flags/data                                                      |
  |   44   |   4  |                      | (reserved)        | (reserved for use in future versions)                                           |
  |   48   |  80  | uint8_t[80]          | name              | file/directory name, limited to 80 bytes                                        |
  |--------|------|----------------------|-------------------|---------------------------------------------------------------------------------|
  |        | 128  |                      |                   | **Entry Total**                                                                 |

  **Self Directory Entry**

  | offset | size | data type            | name              | description                                                                    |
  |--------|------|----------------------|-------------------|--------------------------------------------------------------------------------|
  |    0   |   1  | directory_entry_type | type              | type of entry this is: `0x01`                                                  |
  |    1   |   1  | uint8_t              | user              | (only used by root directory)                                                  |
  |    2   |   1  | uint8_t              | group             | (only used by root directory)                                                  |
  |    3   |   1  | uint8_t              | permissions       | (only used by root directory)                                                  |
  |    4   |   4  | uint32_t             | block_self        | starting block of self (used for identifying self in parent table)             |
  |    8   |   4  | uint32_t             | block_parent      | starting block of parent directory                                             |
  |   12   |   4  | uint32_t             | size_blocks       | (only used by root directory)                                                  |
  |   16   |   8  | uint64_t             | time_created      | (only used by root directory)                                                  |
  |   24   |   8  | uint64_t             | time_modified     | (only used by root directory)                                                  |
  |   32   |   4  | uint32_t             | attribute_block   | (only used by root directory)                                                  |
  |   36   |   4  | uint32_t             | attribute_size    | (only used by root directory)                                                  |
  |   40   |   4  | uint32_t             | system_flags      | (only used by root directory)                                                  |
  |   44   |   4  |                      | (reserved)        | (reserved for use in future versions)                                          |
  |   48   |  80  | uint8_t[80]          | name              | self directory name; duplicated here from parent directory entry               |
  |--------|------|----------------------|-------------------|--------------------------------------------------------------------------------|
  |        | 128  |                      |                   | **Entry Total**                                                                |

Limits by Filesystem Size
--------------------------------------------------------------------------------
  - 4 GiB Filesystem
    - 4 GiB / 512 B block => 8 MiBlocks
    - 8 MiBlocks * 4 Bytes => 32 MiB Chain Table
    - Maximum of < 8 MiFiles
  - 256 MiB Filesystem
    - 256 MiB / 512 B block => 512 KiBlocks
    - 512 KiBlocks * 4 Bytes => 2 MiB Chain Table
    - Maximum of < 512 KiFiles
  - *x* bytes
    - x / 512 => number_of_blocks
    - number_of_blocks * 4 => chain_table_size
    - maximum number of possible files/directories <= depends on use, but less than number_of_blocks

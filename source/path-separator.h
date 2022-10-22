#ifndef BULGE_PATH_SEPARATOR_HEADER
#define BULGE_PATH_SEPARATOR_HEADER

#include <stdint.h>
#include <stdbool.h>

typedef struct structBulgePathSeparator {
  uint8_t* current_entry_begin;
  uint8_t* current_entry_end;
  bool     complete;
} BulgePathSeparator;

bool bulgePathSeparator_reset(BulgePathSeparator* separator, uint8_t* path);
bool bulgePathSeparator_nextComponent(BulgePathSeparator* separator);

#endif // ifndef BULGE_PATH_SEPARATOR_HEADER

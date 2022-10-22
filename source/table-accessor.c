#include <bulge/bulge.h>

void bulgeTableAccessor_reset(BulgeTableAccessor* accessor, uint8_t* block_buffer) {
  accessor->block_current = 0;
  accessor->block_buffer  = block_buffer;
  accessor->dirty         = false;
}

BulgeError bulgeTableAccessor_flush(BulgeFilesystem* filesystem, BulgeTableAccessor* accessor) {
  if(filesystem->write(filesystem->block_filesystem + accessor->block_current, accessor->block_buffer, filesystem->callback_data) == false) {
    return BULGE_ERROR_DEVICE_WRITE;
  }

  accessor->dirty = false;
  return BULGE_ERROR_NONE;
}

BulgeError bulgeTableAccessor_getEntry(BulgeFilesystem* filesystem, BulgeTableAccessor* accessor, uint32_t table_index, uint32_t* table_value) {
  uint32_t block_needed = 1 + (table_index >> 7); // 128 entries per block; so block_needed = index/128

  // if needed, flush old block, read new block
  if(accessor->block_current != block_needed) {
    if(accessor->dirty) {
      BulgeError error = bulgeTableAccessor_flush(filesystem, accessor);
      if(error > BULGE_ERROR_NONE) { return error; }
    }
    if(filesystem->read(filesystem->block_filesystem + block_needed, accessor->block_buffer, filesystem->callback_data) == false) {
      return BULGE_ERROR_DEVICE_READ;
    }
    accessor->block_current = block_needed;
  }

  *table_value = BULGE_ENDIAN32(((uint32_t*)(accessor->block_buffer))[table_index & 127]);
  return BULGE_ERROR_NONE;
}

BulgeError bulgeTableAccessor_setEntry(BulgeFilesystem* filesystem, BulgeTableAccessor* accessor, uint32_t table_index, uint32_t table_value) {
  uint32_t block_needed = 1 + (table_index >> 7); // 128 entries per block; so block_needed = index/128

  // load needed block if required, flushing old block as required
  if(accessor->block_current != block_needed) {
    if(accessor->dirty) {
      BulgeError error = bulgeTableAccessor_flush(filesystem, accessor);
      if(error > BULGE_ERROR_NONE) { return error; }
    }
    if(filesystem->read(filesystem->block_filesystem + block_needed, accessor->block_buffer, filesystem->callback_data) == false) {
      return BULGE_ERROR_DEVICE_READ;
    }
    accessor->block_current = block_needed;
  }

  ((uint32_t*)(accessor->block_buffer))[table_index & 127] = BULGE_ENDIAN32(table_value);
  accessor->dirty = true;
  return BULGE_ERROR_NONE;
}

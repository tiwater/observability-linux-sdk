//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Memory buffer based IO implementations.

#include "core_elf_memory_io.h"

#include <string.h>

#include "ticos/core/math.h"

static ssize_t prv_mio_read(const struct TicosCoreElfReadIO *io, void *buffer,
                            size_t buffer_size) {
  sTicosCoreElfReadMemoryIO *mio = (sTicosCoreElfReadMemoryIO *)io;
  const ssize_t size_to_read = TICOS_MIN((ssize_t)buffer_size, mio->end - mio->cursor);
  const ssize_t size = TICOS_MIN(size_to_read, mio->next_read_size);
  if (buffer != NULL) {
    memcpy(buffer, mio->cursor, size);
  }
  mio->cursor += size;
  return size;
}

void ticos_core_elf_read_memory_io_init(sTicosCoreElfReadMemoryIO *mio, uint8_t *buffer,
                                           size_t buffer_size) {
  *mio = (sTicosCoreElfReadMemoryIO){
    .io =
      {
        .read = prv_mio_read,
      },
    .buffer = buffer,
    .end = buffer + buffer_size,
    .cursor = buffer,
    // Default to reading one byte at a time:
    .next_read_size = 1,
  };
}

static ssize_t prv_mio_write(sTicosCoreElfWriteIO *io, const void *data, size_t size) {
  sTicosCoreElfWriteMemoryIO *mio = (sTicosCoreElfWriteMemoryIO *)io;
  if (mio->cursor + size > mio->end) {
    return -1;
  }
  memcpy(mio->cursor, data, size);
  mio->cursor += size;
  return (ssize_t)size;
}

static bool prv_mio_sync(const sTicosCoreElfWriteIO *io) { return true; }

void ticos_core_elf_write_memory_io_init(sTicosCoreElfWriteMemoryIO *mio, void *buffer,
                                            size_t buffer_size) {
  *mio = (sTicosCoreElfWriteMemoryIO){
    .io =
      {
        .write = prv_mio_write,
        .sync = prv_mio_sync,
      },
    .buffer = buffer,
    .end = ((uint8_t *)buffer) + buffer_size,
    .cursor = (uint8_t *)buffer,
  };
}

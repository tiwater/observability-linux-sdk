#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Memory buffer based IO implementations.

#include "coredump/core_elf_reader.h"
#include "coredump/core_elf_writer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TicosCoreElfReadMemoryIO {
  sTicosCoreElfReadIO io;
  uint8_t *buffer;
  uint8_t *end;
  uint8_t *cursor;
  ssize_t next_read_size;
} sTicosCoreElfReadMemoryIO;

void ticos_core_elf_read_memory_io_init(sTicosCoreElfReadMemoryIO *mio, uint8_t *buffer,
                                           size_t buffer_size);

typedef struct TicosCoreElfWriteMemoryIO {
  sTicosCoreElfWriteIO io;
  void *buffer;
  uint8_t *end;
  uint8_t *cursor;
} sTicosCoreElfWriteMemoryIO;

void ticos_core_elf_write_memory_io_init(sTicosCoreElfWriteMemoryIO *mio, void *buffer,
                                            size_t buffer_size);

#ifdef __cplusplus
}
#endif

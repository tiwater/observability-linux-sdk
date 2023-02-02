#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ELF coredump writer

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zlib.h>

#include "core_elf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interface for an IO object to which to write to using sTicosCoreElfWriter.
 */
typedef struct TicosCoreElfWriteIO {
  /**
   * Called whenever sTicosCoreElfWriter needs to write ELF data. It is called as part of the
   * ticos_core_elf_writer_write() function. This function is expected to follow
   * the same semantics as the write(2) Linux standard library call.
   *
   * @param io The sTicosCoreElfWriteIO itself.
   * @param data Buffer with data to write.
   * @param size The size of the buffer.
   * @return The number of bytes written, or -1 in case an error occurred. errno is expected to be
   * set with the error code.
   */
  ssize_t (*write)(struct TicosCoreElfWriteIO *io, const void *data, size_t size);
  bool (*sync)(const struct TicosCoreElfWriteIO *io);
} sTicosCoreElfWriteIO;

/**
 * Callback that is called for a segment that was added using
 * ticos_core_elf_writer_add_segment_with_callback(). It is called as part
 * of the ticos_core_elf_writer_write() function. The
 * ticos_core_elf_writer_write_segment_data() function must be called one or multiple times
 * within this callback to write the segment data.
 * @param ctx The user-specified ctx pointer that was passed to
 * ticos_core_elf_writer_add_segment_with_callback().
 * @param segment The segment header for which to write data.
 * @return True if writing the data succeeded or false if not.
 */
typedef bool (*TicosCoreWriterSegmentDataCallback)(void *ctx, const Elf_Phdr *segment);

/**
 * Internal data structure.
 */
typedef struct TicosCoreElfWriterSegment {
  Elf_Phdr header;
  union {
    void *data;
    struct {
      TicosCoreWriterSegmentDataCallback fn;
      void *ctx;
    } callback;
  };
  bool has_callback;
} sTicosCoreElfWriterSegment;

/**
 * Minimalistic, ELF coredump writer. It is designed to write out the coredump sequentially (not
 * requiring seeking). This makes it possible to write the data on-the-fly to a streaming
 * compression algorithm.
 */
typedef struct TicosCoreElfWriter {
  sTicosCoreElfWriteIO *io;
  Elf_Half e_machine;
  Elf_Word e_flags;
  sTicosCoreElfWriterSegment *segments;
  size_t segments_max;
  int segments_idx;
  Elf64_Off write_offset;
} sTicosCoreElfWriter;

/**
 * Initializes a sTicosCoreElfWriter.
 * @param writer The writer to initialize.
 * @param io The IO to initialize the writer with.
 */
void ticos_core_elf_writer_init(sTicosCoreElfWriter *writer, sTicosCoreElfWriteIO *io);

/**
 * Sets the e_machine and e_flags to use in the ELF file header.
 * @param writer The writer.
 * @param e_machine The e_machine flag value.
 * @param e_flags The e_flags value.
 */
void ticos_core_elf_writer_set_elf_header_fields(sTicosCoreElfWriter *writer,
                                                    Elf_Half e_machine, Elf_Word e_flags);

/**
 * Adds a segment to the writer using a buffer to provide the segment data.
 * @note ownership over segment_data is transferred. The writer will free the data as part of
 * ticos_core_elf_writer_finalize().
 * @param writer The writer.
 * @param segment The segment header. The e_filesz should be filled with the size of the
 * segment_data buffer. The e_offset field is automatically filled in.
 * @param segment_data The buffer with the data of the segment.
 * @return True if the segment was added successfully, or false if not.
 */
bool ticos_core_elf_writer_add_segment_with_buffer(sTicosCoreElfWriter *writer,
                                                      const Elf_Phdr *segment, void *segment_data);

/**
 * Adds a segment to the writer using a callback to provide the segment data.
 * @param writer The writer.
 * @param segment The segment header. The e_filesz should be filled with the size of the
 * segment_data that will be written by the callback. The e_offset field is automatically filled in.
 * @param segment_data_cb The callback that will be called later, as part of the
 * ticos_core_elf_writer_write(), to write the segment data.
 * @param ctx User-specified context pointer that will be passed to the segment_data_cb.
 * @return True if the segment was added successfully, or false if not.
 */
bool ticos_core_elf_writer_add_segment_with_callback(
  sTicosCoreElfWriter *writer, const Elf_Phdr *segment,
  TicosCoreWriterSegmentDataCallback segment_data_cb, void *ctx);

/**
 * Writes the entire ELF file to the IO interface.
 * @param writer The writer.
 * @return True if the write was successful or false if not.
 */
bool ticos_core_elf_writer_write(sTicosCoreElfWriter *writer);

/**
 * Writes segment data. To be called from within a TicosCoreWriterSegmentDataCallback.
 * See ticos_core_elf_writer_add_segment_with_callback().
 * @param writer The writer.
 * @param data Buffer with the data to write.
 * @param size Size of the data buffer.
 * @return True if the write was successful or false if not.
 */
bool ticos_core_elf_writer_write_segment_data(sTicosCoreElfWriter *writer, const void *data,
                                                 size_t size);

/**
 * Frees any resources allocated by the writer.
 * @param writer The writer
 */
void ticos_core_elf_writer_finalize(sTicosCoreElfWriter *writer);

/**
 * Object that implements the sTicosCoreElfWriteIO interface by writing to a file descriptor.
 */
typedef struct TicosCoreElfWriteFileIO {
  sTicosCoreElfWriteIO io;
  int fd;
  size_t max_size;
  size_t written_size;
} sTicosCoreElfWriteFileIO;

/**
 * Initializes a sTicosCoreElfWriteFileIO.
 * @param fio The sTicosCoreElfWriteFileIO object to initialize.
 * @param fd The file descriptor to write to.
 */
void ticos_core_elf_write_file_io_init(sTicosCoreElfWriteFileIO *fio, int fd,
                                          size_t max_size);

/**
 * Object that implements the sTicosCoreElfWriteIO interface by compressing the data using gzip
 * and then calling another sTicosCoreElfWriteIO interface with the compressed data.
 */
typedef struct TicosCoreElfWriteGzipIO {
  sTicosCoreElfWriteIO io;
  sTicosCoreElfWriteIO *next;
  z_stream zs;
} sTicosCoreElfWriteGzipIO;

/**
 * Initializes a sTicosCoreElfWriteGzipIO.
 * @param gzio The sTicosCoreElfWriteGzipIO object to initialize.
 * @param next The sTicosCoreElfWriteIO object that should be called to write out the compressed
 * data.
 * @return True if the initialization was successful, or false in case of an error.
 */
bool ticos_core_elf_write_gzip_io_init(sTicosCoreElfWriteGzipIO *gzio,
                                          sTicosCoreElfWriteIO *next);

/**
 * De-initializes a sTicosCoreElfWriteGzipIO, releasing its resources.
 * @param gzio The sTicosCoreElfWriteGzipIO object to de-initialize.
 * @return True if the de-initialization was successful, or false in case of an error.
 */
bool ticos_core_elf_write_gzip_io_deinit(sTicosCoreElfWriteGzipIO *gzio);

#ifdef __cplusplus
}
#endif

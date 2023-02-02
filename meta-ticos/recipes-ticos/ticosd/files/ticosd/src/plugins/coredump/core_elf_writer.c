//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ELF coredump writer

#include "core_elf_writer.h"

#include <errno.h>
#include <link.h>
#include <malloc.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "ticos/core/math.h"

#define SEGMENTS_REALLOC_STEP_SIZE (32)
#define PADDING_WRITE_SIZE (4096)
#define GZIP_COMPRESSION_BUFFER_SIZE (4096)

void ticos_core_elf_writer_init(sTicosCoreElfWriter *writer, sTicosCoreElfWriteIO *io) {
  *writer = (sTicosCoreElfWriter){
    .io = io,
    .segments = NULL,
    .segments_max = 0,
    .segments_idx = -1,
  };
}

void ticos_core_elf_writer_set_elf_header_fields(sTicosCoreElfWriter *writer,
                                                    Elf_Half e_machine, Elf_Word e_flags) {
  writer->e_machine = e_machine;
  writer->e_flags = e_flags;
}

static sTicosCoreElfWriterSegment *prv_alloc_next_segment(sTicosCoreElfWriter *writer) {
  if (writer->segments_idx >= (int)writer->segments_max - 1) {
    writer->segments_max += SEGMENTS_REALLOC_STEP_SIZE;
    sTicosCoreElfWriterSegment *const new_segments =
      realloc(writer->segments, sizeof(sTicosCoreElfWriterSegment) * writer->segments_max);
    if (new_segments == NULL) {
      return NULL;
    }
    writer->segments = new_segments;
  }
  return &writer->segments[++writer->segments_idx];
}

static bool prv_io_write_all(sTicosCoreElfWriteIO *io, const void *data, size_t size) {
  size_t bytes_written = 0;

  while (bytes_written < size) {
    const ssize_t rv = io->write(io, ((const uint8_t *)data) + bytes_written, size - bytes_written);
    if (rv < 0) {
      if (errno == EINTR) {
        continue;
      }
      fprintf(stderr, "core_elf:: failed to write: %s\n", strerror(errno));
      return false;
    }
    bytes_written += rv;
  }
  return true;
}

static bool prv_write_all(sTicosCoreElfWriter *writer, const void *data, size_t size) {
  if (!prv_io_write_all(writer->io, data, size)) {
    return false;
  }
  writer->write_offset += size;
  return true;
}

static size_t prv_calc_padding(size_t offset, const Elf_Phdr *segment) {
  // Calculate padding needed, to abide by the segment's p_align.
  // As per the spec: "This member gives the value to which the segments are aligned in memory and
  // in the file. Values 0 and 1 mean that no alignment is required."
  if (segment->p_align <= 1) {
    return 0;
  }
  return TICOS_ALIGN_UP(offset, segment->p_align) - offset;
}

static bool prv_write_padding(sTicosCoreElfWriter *writer, size_t pad_size) {
  if (pad_size == 0) {
    return true;
  }
  uint8_t zeroes[PADDING_WRITE_SIZE] = {0};
  while (pad_size > 0) {
    const size_t sz = MIN(sizeof(zeroes), pad_size);
    if (!prv_write_all(writer, zeroes, sz)) {
      return false;
    }
    pad_size -= sz;
  }
  return true;
}

bool ticos_core_elf_writer_add_segment_with_buffer(sTicosCoreElfWriter *writer,
                                                      const Elf_Phdr *segment, void *segment_data) {
  sTicosCoreElfWriterSegment *const segment_wrapper = prv_alloc_next_segment(writer);
  if (segment_wrapper == NULL) {
    return false;
  }
  *segment_wrapper = (sTicosCoreElfWriterSegment){
    .header = *segment,
    .data = segment_data,
    .has_callback = false,
  };
  return true;
}

bool ticos_core_elf_writer_add_segment_with_callback(
  sTicosCoreElfWriter *writer, const Elf_Phdr *segment,
  TicosCoreWriterSegmentDataCallback segment_data_cb, void *ctx) {
  sTicosCoreElfWriterSegment *const segment_wrapper = prv_alloc_next_segment(writer);
  if (segment_wrapper == NULL) {
    return false;
  }
  *segment_wrapper = (sTicosCoreElfWriterSegment){
    .header = *segment,
    .callback =
      {
        .fn = segment_data_cb,
        .ctx = ctx,
      },
    .has_callback = true,
  };
  return true;
}

bool ticos_core_elf_writer_write_segment_data(sTicosCoreElfWriter *writer, const void *data,
                                                 size_t size) {
  return prv_write_all(writer, data, size);
}

bool ticos_core_elf_writer_write(sTicosCoreElfWriter *writer) {
  bool result;
  const size_t num_segments = writer->segments_idx + 1;

  // Write ELF header:
  const Elf_Ehdr elf_header = (Elf_Ehdr){
    .e_ident =
      {
        ELFMAG0,
        ELFMAG1,
        ELFMAG2,
        ELFMAG3,
        ELFCLASS,
        ELFDATA,
        EV_CURRENT,
      },
    .e_type = ET_CORE,
    .e_machine = writer->e_machine,
    .e_version = EV_CURRENT,
    .e_entry = 0,
    // Segment header table is written immediately after the ELF header:
    .e_phoff = num_segments > 0 ? sizeof(elf_header) : 0,
    .e_shoff = 0,
    .e_flags = writer->e_flags,
    .e_ehsize = sizeof(Elf_Ehdr),
    .e_phentsize = num_segments > 0 ? sizeof(Elf_Phdr) : 0,
    .e_phnum = num_segments,
    .e_shentsize = 0,
    .e_shnum = 0,
    .e_shstrndx = 0,
  };
  result = prv_write_all(writer, &elf_header, sizeof(elf_header));
  if (!result) {
    goto cleanup;
  }

  // Write segment table:
  size_t segment_data_offset = writer->write_offset + sizeof(Elf_Phdr) * num_segments;
  for (unsigned int i = 0; i < num_segments; ++i) {
    sTicosCoreElfWriterSegment *const wrapper = &writer->segments[i];
    const size_t pad_size = prv_calc_padding(segment_data_offset, &wrapper->header);
    wrapper->header.p_offset = segment_data_offset + pad_size;
    segment_data_offset += wrapper->header.p_filesz + pad_size;
    result = prv_write_all(writer, &wrapper->header, sizeof(wrapper->header));
    if (!result) {
      goto cleanup;
    }
  }

  // Write segment data blocks:
  for (unsigned int i = 0; i < num_segments; ++i) {
    sTicosCoreElfWriterSegment *const wrapper = &writer->segments[i];
    const size_t pad_size = wrapper->header.p_offset - writer->write_offset;
    result = prv_write_padding(writer, pad_size);
    if (!result) {
      goto cleanup;
    }

    if (wrapper->has_callback) {
      result = wrapper->callback.fn(wrapper->callback.ctx, &wrapper->header);
    } else {
      result = prv_write_all(writer, wrapper->data, wrapper->header.p_filesz);
    }
    if (!result) {
      goto cleanup;
    }

    if (writer->write_offset != wrapper->header.p_offset + wrapper->header.p_filesz) {
      fprintf(stderr, "Written segment data end (0x%lx) did not match planned end (0x%lx)",
              (unsigned long)writer->write_offset,
              (unsigned long)wrapper->header.p_offset + wrapper->header.p_filesz);
      result = false;
    }
    if (!result) {
      goto cleanup;
    }
  }

  result = writer->io->sync(writer->io);

cleanup:
  return result;
}

void ticos_core_elf_writer_finalize(sTicosCoreElfWriter *writer) {
  const size_t num_segments = writer->segments_idx + 1;
  for (unsigned int i = 0; i < num_segments; ++i) {
    sTicosCoreElfWriterSegment *const wrapper = &writer->segments[i];
    if (!wrapper->has_callback) {
      free(wrapper->data);
    }
  }
  free(writer->segments);
}

static ssize_t prv_fio_write(sTicosCoreElfWriteIO *io, const void *data, size_t size) {
  sTicosCoreElfWriteFileIO *fio = (sTicosCoreElfWriteFileIO *)io;
  if (fio->written_size + size > fio->max_size) {
    fprintf(stderr, "core_elf:: cannot write corefile, max size reached\n");
    return -1;
  }
  const size_t bytes = write(fio->fd, data, size);
  fio->written_size += bytes;
  return bytes;
}

static bool prv_fio_sync(const sTicosCoreElfWriteIO *io) {
  const sTicosCoreElfWriteFileIO *fio = (const sTicosCoreElfWriteFileIO *)io;
  const bool success = fsync(fio->fd) != -1;
  if (!success) {
    fprintf(stderr, "core_elf:: fsync failed: %s\n", strerror(errno));
  }
  return success;
}

void ticos_core_elf_write_file_io_init(sTicosCoreElfWriteFileIO *fio, int fd,
                                          size_t max_size) {
  *fio = (sTicosCoreElfWriteFileIO){
    .io =
      {
        .write = prv_fio_write,
        .sync = prv_fio_sync,
      },
    .fd = fd,
    .max_size = max_size,
  };
}

static ssize_t prv_write(struct TicosCoreElfWriteIO *io, const void *data, size_t size) {
  sTicosCoreElfWriteGzipIO *const gzio = (sTicosCoreElfWriteGzipIO *)io;
  gzio->zs.next_in = (unsigned char *)data;
  gzio->zs.avail_in = size;

  uint8_t buffer[GZIP_COMPRESSION_BUFFER_SIZE];
  while (gzio->zs.avail_in > 0) {
    gzio->zs.next_out = buffer;
    gzio->zs.avail_out = sizeof(buffer);
    const int rv = deflate(&gzio->zs, Z_NO_FLUSH);
    if (rv != Z_OK) {
      fprintf(stderr, "core_elf:: deflate error: %d\n", rv);
      return -1;
    }
    if (!prv_io_write_all(gzio->next, buffer, gzio->zs.next_out - buffer)) {
      return -1;
    }
  }
  return (ssize_t)size;
}

static bool prv_sync(const struct TicosCoreElfWriteIO *io) {
  sTicosCoreElfWriteGzipIO *const gzio = (sTicosCoreElfWriteGzipIO *)io;
  uint8_t buffer[GZIP_COMPRESSION_BUFFER_SIZE];
  while (true) {
    gzio->zs.next_out = buffer;
    gzio->zs.avail_out = sizeof(buffer);
    const int rv = deflate(&gzio->zs, Z_FINISH);
    if (rv != Z_OK && rv != Z_STREAM_END) {
      fprintf(stderr, "core_elf:: deflate error: %d\n", rv);
      return false;
    }
    if (!prv_io_write_all(gzio->next, buffer, gzio->zs.next_out - buffer)) {
      return false;
    }
    if (rv == Z_STREAM_END) {
      return true;
    }
  }
}

bool ticos_core_elf_write_gzip_io_init(sTicosCoreElfWriteGzipIO *gzio,
                                          sTicosCoreElfWriteIO *next) {
  *gzio = (sTicosCoreElfWriteGzipIO){
    .io =
      {
        .write = prv_write,
        .sync = prv_sync,
      },
    .next = next,
    .zs =
      {
        .next_in = Z_NULL,
        .zalloc = Z_NULL,
        .zfree = Z_NULL,
        .opaque = Z_NULL,
      },
  };

  // We'll be using ~256K of memory with the default configuration. As per the zconf.h:
  // "The memory requirements for deflate are (in bytes)
  // (1 << (windowBits+2)) +  (1 << (memLevel+9))"
  const int mem_level = 8;     // default
  const int window_bits = 15;  // default
  // "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data
  // instead of a zlib wrapper":
  const int gzip_header_inc = 16;
  const int rv = deflateInit2(&gzio->zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                              window_bits + gzip_header_inc, mem_level, Z_DEFAULT_STRATEGY);
  if (rv != Z_OK) {
    fprintf(stderr, "core_elf:: deflateInit2 error %d\n", rv);
    return false;
  }
  return true;
}

bool ticos_core_elf_write_gzip_io_deinit(sTicosCoreElfWriteGzipIO *gzio) {
  const int rv = deflateEnd(&gzio->zs);
  if (rv != Z_OK) {
    fprintf(stderr, "core_elf:: deflateEnd error: %d\n", rv);
    return false;
  }
  return true;
}

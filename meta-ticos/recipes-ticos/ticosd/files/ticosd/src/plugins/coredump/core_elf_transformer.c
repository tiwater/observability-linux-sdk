//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ELF coredump transformer

// for 64-bit pread() in prv_copy_proc_mem:
#define _FILE_OFFSET_BITS 64

#include "core_elf_transformer.h"

#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "ticos/core/math.h"
#include "ticos/util/string.h"

static sTicosCoreElfTransformer *prv_cast_reader_to_transformer(sTicosCoreElfReader *reader) {
  sTicosCoreElfTransformer *transformer =
    (sTicosCoreElfTransformer *)((uint8_t *)reader -
                                    offsetof(sTicosCoreElfTransformer, reader));
  return transformer;
}

static void prv_add_warning(sTicosCoreElfTransformer *transformer, char *warning_msg) {
  if (warning_msg == NULL) {
    fprintf(stderr, "core_elf_transformer:: received NULL warning message\n");
    return;
  }
  fprintf(stderr, "core_elf_transformer:: received warning: %s\n", warning_msg);
  if (transformer->next_warning_idx > TICOS_ARRAY_SIZE(transformer->warnings)) {
    fprintf(stderr, "core_elf_transformer:: dropping warning: %s\n", warning_msg);
    free(warning_msg);
    return;
  }
  transformer->warnings[transformer->next_warning_idx++] = warning_msg;
}

static void prv_handle_warning(sTicosCoreElfReader *reader, char *warning_msg) {
  sTicosCoreElfTransformer *transformer = prv_cast_reader_to_transformer(reader);
  prv_add_warning(transformer, warning_msg);
}

static void prv_handle_elf_header(sTicosCoreElfReader *reader, const Elf_Ehdr *elf_header) {
  sTicosCoreElfTransformer *transformer = prv_cast_reader_to_transformer(reader);
  ticos_core_elf_writer_set_elf_header_fields(&transformer->writer, elf_header->e_machine,
                                                 elf_header->e_flags);
}

static void prv_process_note_segment(sTicosCoreElfReader *reader, const Elf_Phdr *segment) {
  sTicosCoreElfTransformer *transformer = prv_cast_reader_to_transformer(reader);

  const size_t note_segment_buffer_size = segment->p_filesz;
  uint8_t *note_buffer = malloc(note_segment_buffer_size);
  if (note_buffer == NULL) {
    char *warning = NULL;
    ticos_asprintf(&warning, "Failed to allocate %lu bytes for note buffer",
                      (unsigned long)note_segment_buffer_size);
    prv_add_warning(transformer, warning);
    return;
  }

  const size_t size_read = ticos_core_elf_reader_read_segment_data(
    reader, segment->p_offset, note_buffer, segment->p_filesz);
  if (size_read != segment->p_filesz) {
    char *warning = NULL;
    ticos_asprintf(&warning, "Failed to read note at %lu (%lu bytes)",
                      (unsigned long)segment->p_offset, (unsigned long)note_segment_buffer_size);
    prv_add_warning(transformer, warning);
    return;
  }

  // FUTURE: parse NT_PRSTATUS notes to get the stack pointers for each thread and use it to (only)
  // capture stack memory.

  if (!ticos_core_elf_writer_add_segment_with_buffer(&transformer->writer, segment,
                                                        note_buffer)) {
    prv_add_warning(transformer, strdup("Failed to add note to writer"));
  }
}

static bool prv_write_load_segment_data_cb(void *ctx, const Elf_Phdr *segment) {
  sTicosCoreElfTransformer *transformer = (sTicosCoreElfTransformer *)ctx;

  // Copy over segment data from /proc/<pid>/mem in chunks and pass them to the writer:
  uint8_t buffer[TICOS_CORE_ELF_TRANSFORMER_PROC_MEM_COPY_BUFFER_SIZE_BYTES];
  Elf_Addr vaddr = segment->p_vaddr;
  Elf64_Xword bytes_remaining = segment->p_filesz;
  while (bytes_remaining > 0) {
    Elf64_Xword sz = TICOS_MIN(bytes_remaining, sizeof(buffer));
    ssize_t bytes_read = transformer->transformer_handler->copy_proc_mem(
      transformer->transformer_handler, vaddr, sz, buffer);
    if (bytes_read <= 0) {
      // Read error or EOF. Keep going, but fill the buffer with placeholder bytes:
      memset(buffer, 0xEF, sz);
      bytes_read = (ssize_t)sz;
    }
    if (!ticos_core_elf_writer_write_segment_data(&transformer->writer, buffer, bytes_read)) {
      return false;
    }
    bytes_remaining -= bytes_read;
    vaddr += bytes_read;
  }
  return true;
}

static bool prv_process_load_segment(sTicosCoreElfReader *reader,
                                     const Elf_Phdr *segment_header) {
  sTicosCoreElfTransformer *transformer = prv_cast_reader_to_transformer(reader);

  // FUTURE: filter by breaking down segments to keep only pieces of interest,
  // for now keep each LOAD segment as-is:
  return ticos_core_elf_writer_add_segment_with_callback(
    &transformer->writer, segment_header, prv_write_load_segment_data_cb, transformer);
}

static void prv_append_ticos_metadata_note(sTicosCoreElfTransformer *transformer) {
  // TODO: Ticos-7205 Add warnings to metadata ELF note
  const size_t note_buffer_size =
    ticos_core_elf_metadata_note_calculate_size(transformer->metadata);
  uint8_t *note_buffer = malloc(note_buffer_size);
  if (note_buffer == NULL) {
    fprintf(stderr, "core_elf_transformer:: allocate note buffer of %lu bytes\n",
            (unsigned long)note_buffer_size);
    return;
  }
  if (!ticos_core_elf_metadata_note_write(transformer->metadata, note_buffer,
                                             note_buffer_size)) {
    fprintf(stderr, "core_elf_transformer:: failed to add metadata to note\n");
    return;
  }
  const Elf_Phdr segment = {
    .p_type = PT_NOTE,
    .p_filesz = note_buffer_size,
  };
  if (!ticos_core_elf_writer_add_segment_with_buffer(&transformer->writer, &segment,
                                                        note_buffer)) {
    fprintf(stderr, "core_elf_transformer:: failed to add metadata note to writer\n");
  }
}

static void prv_handle_segments(sTicosCoreElfReader *reader, const Elf_Phdr *segments,
                                size_t num_segments) {
  sTicosCoreElfTransformer *transformer = prv_cast_reader_to_transformer(reader);

  for (size_t i = 0; i < num_segments; ++i) {
    const Elf_Phdr *segment = &segments[i];
    switch (segment->p_type) {
      case PT_NOTE:
        prv_process_note_segment(reader, segment);
        break;
      case PT_LOAD:
        prv_process_load_segment(reader, segment);
        break;
      default: {
        // core.elf files generated by the kernel only contain NOTE and LOAD segments,
        // no need to handle other types, but emit a warning in case this changes in the future:
        char *warning = NULL;
        ticos_asprintf(&warning, "Unexpected segment type: %u", segment->p_type);
        prv_add_warning(transformer, warning);
        break;
      }
    }
  }

  // Add the metadata note last. This way any warnings that got added while handling the segments
  // get included in the metadata blob:
  prv_append_ticos_metadata_note(transformer);

  // Write out the ELF:
  transformer->write_success = ticos_core_elf_writer_write(&transformer->writer);
}

static void prv_free_warnings(sTicosCoreElfTransformer *transformer) {
  for (size_t i = 0; i < transformer->next_warning_idx; ++i) {
    free(transformer->warnings[i]);
  }
}

static void prv_handle_done(sTicosCoreElfReader *reader) {
  sTicosCoreElfTransformer *transformer = prv_cast_reader_to_transformer(reader);
  ticos_core_elf_writer_finalize(&transformer->writer);
  prv_free_warnings(transformer);
}

void ticos_core_elf_transformer_init(sTicosCoreElfTransformer *transformer,
                                        sTicosCoreElfReadIO *reader_io,
                                        sTicosCoreElfWriteIO *writer_io,
                                        const sTicosCoreElfMetadata *metadata,
                                        sTicosCoreElfTransformerHandler *transformer_handler) {
  *transformer = (sTicosCoreElfTransformer){
    .read_handler =
      {
        .handle_elf_header = prv_handle_elf_header,
        .handle_segments = prv_handle_segments,
        .handle_warning = prv_handle_warning,
        .handle_done = prv_handle_done,
      },
    .metadata = metadata,
    .transformer_handler = transformer_handler,
    .write_success = false,
  };
  ticos_core_elf_reader_init(&transformer->reader, reader_io, &transformer->read_handler);
  ticos_core_elf_writer_init(&transformer->writer, writer_io);
}

bool ticos_core_elf_transformer_run(sTicosCoreElfTransformer *transformer) {
  return ticos_core_elf_reader_read_all(&transformer->reader) && transformer->write_success;
}

static ssize_t prv_copy_proc_mem(sTicosCoreElfTransformerHandler *handler, Elf_Addr vaddr,
                                 Elf64_Xword size, void *buffer) {
  sTicosCoreElfTransformerProcfsHandler *procfs_handler =
    (sTicosCoreElfTransformerProcfsHandler *)handler;
  TICOS_STATIC_ASSERT(sizeof(__off_t) == sizeof(Elf_Addr), "");
  return pread(procfs_handler->fd, buffer, size, vaddr);
}

bool ticos_init_core_elf_transformer_procfs_handler(
  sTicosCoreElfTransformerProcfsHandler *handler, pid_t pid) {
  char procfs_path[128];
  snprintf(procfs_path, sizeof(procfs_path), "/proc/%d/mem", pid);
  const int fd = open(procfs_path, O_RDONLY | O_CLOEXEC);
  *handler = (sTicosCoreElfTransformerProcfsHandler){
    .handler =
      {
        .copy_proc_mem = prv_copy_proc_mem,
      },
    .fd = fd,
  };
  if (fd == -1) {
    fprintf(stderr, "core_elf_transformer:: failed to open %s: %s\n", procfs_path, strerror(errno));
    return false;
  }
  return true;
}

bool ticos_deinit_core_elf_transformer_procfs_handler(
  sTicosCoreElfTransformerProcfsHandler *handler) {
  if (handler->fd == -1) {
    // The file had not been opened in ticos_init_core_elf_transformer_procfs_handler(),
    // no need to close:
    return true;
  }
  return close(handler->fd) == 0;
}

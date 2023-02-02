#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ELF coredump reader

#include <stdbool.h>
#include <stddef.h>

#include "core_elf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks whether the passed buffer contains a valid and supported ELF header of a coredump.
 * @param elf_buffer Pointer to the ELF header.
 * @param buffer_size The size of elf_buffer.
 * @return True if the ELF header is for valid and for a supported ELF coredump.
 */
bool ticos_core_elf_reader_is_valid_core_elf(const void *elf_buffer, size_t buffer_size);

/**
 * Interface for an IO object from which to read using sTicosCoreElfReader.
 */
typedef struct TicosCoreElfReadIO {
  /**
   * Called whenever sTicosCoreElfReader needs to read data. This function is expected to follow
   * the same semantics as the read(2) Linux standard library call.
   * @param io The sTicosCoreElfReadIO itself.
   * @param buffer The buffer in which to copy the read data.
   * @param buffer_size The size of the buffer.
   * @return The number of bytes read, or 0 in case there was an error or the end of the stream
   * (EOF) has been reached, or -1 in case an error occurred.
   */
  ssize_t (*read)(const struct TicosCoreElfReadIO *io, void *buffer, size_t buffer_size);
} sTicosCoreElfReadIO;

typedef struct TicosCoreElfReader sTicosCoreElfReader;

/**
 * Interface for a handler object that will receive parsing callbacks from sTicosCoreElfReader.
 */
typedef struct TicosCoreElfReaderHandler {
  /**
   * Called when the sTicosCoreElfReader has received and validated the ELF header.
   * @param reader The reader itself.
   * @param elf_header The ELF header.
   */
  void (*handle_elf_header)(sTicosCoreElfReader *reader, const Elf_Ehdr *elf_header);
  /**
   * Called when the sTicosCoreElfReader has received the segment header table.
   * @note (Only) from within this callback, ticos_core_elf_reader_read_segment_data() can be
   * used to read the segment data.
   * @param reader The reader itself.
   * @param segments Pointer to the segments array. The array is owned by the reader and will be
   * free'd automatically after the handle_done callback.
   * @param num_segments The number of segments in the array.
   */
  void (*handle_segments)(sTicosCoreElfReader *reader, const Elf_Phdr *segments,
                          size_t num_segments);
  /**
   * @param reader The reader itself.
   * @param msg The warning message or NULL in case there was not enough memory to allocate the
   * message. Ownership is passed to the callback: it must must free(msg).
   */
  void (*handle_warning)(sTicosCoreElfReader *reader, char *msg);
  /**
   * Called when the sTicosCoreElfReader is completed and its resources are about to be freed.
   * @param reader The reader itself.
   */
  void (*handle_done)(sTicosCoreElfReader *reader);
} sTicosCoreElfReaderHandler;

/**
 * Minimalistic, streaming ELF coredump reader. It assumes the segment header table is located
 * immediately after the ELF header and before any segment data. This is how the Linux kernel lays
 * out user-space coredumps.
 */
struct TicosCoreElfReader {
  const sTicosCoreElfReadIO *io;
  sTicosCoreElfReaderHandler *handler;

  /** Current action of the reader. */
  void (*action)(sTicosCoreElfReader *reader);
  /** Position in the stream in number of bytes since the start of the stream */
  size_t stream_pos;
  /** Storage for the ELF header.
   * The contents are only valid between the handle_elf_header and the handle_done calls. */
  Elf_Ehdr elf_header;
  /** Position in bytes into the subelement that is being read from the stream. */
  Elf_Phdr *segments;
};

/**
 * Initializes the reader.
 * @param reader The reader to initialize.
 * @param io The IO object to read from. The position of the IO is expected to be at the beginning
 * of the ELF data.
 * @param handler The handler object that will receive parsing callbacks.
 */
void ticos_core_elf_reader_init(sTicosCoreElfReader *reader, const sTicosCoreElfReadIO *io,
                                   sTicosCoreElfReaderHandler *handler);

/**
 * Starts reading and parsing the ELF coredump from the IO object.
 * @param reader The reader.
 * @return True if it finished reading the coredump, or false if there was an error.
 */
bool ticos_core_elf_reader_read_all(sTicosCoreElfReader *reader);

/**
 * Reads segment data and copy the data into the given buffer. To be called from within a
 * sTicosCoreElfReaderHandler.handle_segments callback. It will start copying from the given
 * stream position, at_pos. It will attempt to copy buffer_size bytes, or less if the end of the
 * file has been reached. If the stream is already ahead of the given stream position, the function
 * will return 0. If the requested stream position is past the end of the stream, the function will
 * return 0.
 * @param reader The reader to read from.
 * @param at_pos The stream position (file offset) from where to start the read.
 * @param buffer the buffer into which to copy the data.
 * @param buffer_size the size of buffer in bytes.
 * @return The number of bytes copied into the buffer.
 */
size_t ticos_core_elf_reader_read_segment_data(sTicosCoreElfReader *reader, size_t at_pos,
                                                  uint8_t *buffer, size_t buffer_size);

/**
 * Object that implements the sTicosCoreElfReadIO interface by reading from a file descriptor.
 */
typedef struct TicosCoreElfReadFileIO {
  sTicosCoreElfReadIO io;
  int fd;
} sTicosCoreElfReadFileIO;

/**
 * Initializes a sTicosCoreElfReadFileIO.
 * @param fio The sTicosCoreElfReadFileIO object to initialize.
 * @param fd The file descriptor to read from.
 */
void ticos_core_elf_read_file_io_init(sTicosCoreElfReadFileIO *fio, int fd);

#ifdef __cplusplus
}
#endif

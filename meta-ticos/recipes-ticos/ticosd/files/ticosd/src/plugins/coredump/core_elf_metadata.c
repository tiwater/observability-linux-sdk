//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Functions to generate Ticos ELF coredump metadata.

#include "core_elf_metadata.h"

#include <stdbool.h>
#include <string.h>

#include "core_elf_note.h"
#include "ticos/util/cbor.h"

static const char s_note_name[] = "Ticos";
static const Elf_Word s_metadata_note_type = 0x4154454d;

static void prv_write_cbor_to_buffer_cb(void *ctx, uint32_t offset, const void *buf,
                                        size_t buf_len) {
  uint8_t *buffer = ctx;
  memcpy(buffer + offset, buf, buf_len);
}

static bool prv_add_schema_version(sTicosCborEncoder *encoder) {
  return (
    ticos_cbor_encode_unsigned_integer(encoder, kTicosCoreElfMetadataKey_SchemaVersion) &&
    ticos_cbor_encode_unsigned_integer(encoder, TICOS_CORE_ELF_METADATA_SCHEMA_VERSION_V1));
}

static bool prv_add_linux_sdk_version(sTicosCborEncoder *encoder,
                                      const char *linux_sdk_version) {
  return (
    ticos_cbor_encode_unsigned_integer(encoder, kTicosCoreElfMetadataKey_LinuxSdkVersion) &&
    ticos_cbor_encode_string(encoder, linux_sdk_version));
}

static bool prv_add_captured_time(sTicosCborEncoder *encoder, uint32_t captured_time_epoch_s) {
  return (
    ticos_cbor_encode_unsigned_integer(encoder, kTicosCoreElfMetadataKey_CapturedTime) &&
    ticos_cbor_encode_unsigned_integer(encoder, captured_time_epoch_s));
}

static bool prv_add_device_serial(sTicosCborEncoder *encoder, const char *device_serial) {
  return (
    ticos_cbor_encode_unsigned_integer(encoder, kTicosCoreElfMetadataKey_DeviceSerial) &&
    ticos_cbor_encode_string(encoder, device_serial));
}

static bool prv_add_hardware_version(sTicosCborEncoder *encoder, const char *hardware_version) {
  return (
    ticos_cbor_encode_unsigned_integer(encoder, kTicosCoreElfMetadataKey_HardwareVersion) &&
    ticos_cbor_encode_string(encoder, hardware_version));
}

static bool prv_add_software_type(sTicosCborEncoder *encoder, const char *software_type) {
  return (
    ticos_cbor_encode_unsigned_integer(encoder, kTicosCoreElfMetadataKey_SoftwareType) &&
    ticos_cbor_encode_string(encoder, software_type));
}

static bool prv_add_software_version(sTicosCborEncoder *encoder, const char *software_version) {
  return (
    ticos_cbor_encode_unsigned_integer(encoder, kTicosCoreElfMetadataKey_SoftwareVersion) &&
    ticos_cbor_encode_string(encoder, software_version));
}

static bool prv_add_cbor_metadata(sTicosCborEncoder *encoder,
                                  const sTicosCoreElfMetadata *metadata) {
  return (ticos_cbor_encode_dictionary_begin(encoder, 7) && prv_add_schema_version(encoder) &&
          prv_add_linux_sdk_version(encoder, metadata->linux_sdk_version) &&
          prv_add_captured_time(encoder, metadata->captured_time_epoch_s) &&
          prv_add_device_serial(encoder, metadata->device_serial) &&
          prv_add_hardware_version(encoder, metadata->hardware_version) &&
          prv_add_software_type(encoder, metadata->software_type) &&
          prv_add_software_version(encoder, metadata->software_version));
}

static size_t prv_cbor_calculate_size(const sTicosCoreElfMetadata *metadata) {
  sTicosCborEncoder encoder;
  ticos_cbor_encoder_size_only_init(&encoder);
  prv_add_cbor_metadata(&encoder, metadata);
  return ticos_cbor_encoder_deinit(&encoder);
}

size_t ticos_core_elf_metadata_note_calculate_size(const sTicosCoreElfMetadata *metadata) {
  return ticos_core_elf_note_calculate_size(s_note_name, prv_cbor_calculate_size(metadata));
}

bool ticos_core_elf_metadata_note_write(const sTicosCoreElfMetadata *metadata,
                                           uint8_t *note_buffer, size_t note_buffer_size) {
  const size_t description_size = prv_cbor_calculate_size(metadata);
  uint8_t *const description_buffer =
    ticos_core_elf_note_init(note_buffer, s_note_name, description_size, s_metadata_note_type);
  const size_t note_header_size = description_buffer - note_buffer;

  sTicosCborEncoder encoder;
  ticos_cbor_encoder_init(&encoder, prv_write_cbor_to_buffer_cb, description_buffer,
                             note_buffer_size - note_header_size);

  return prv_add_cbor_metadata(&encoder, metadata);
}

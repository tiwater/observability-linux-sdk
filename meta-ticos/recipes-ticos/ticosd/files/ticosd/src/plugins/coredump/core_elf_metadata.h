#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Functions to generate Ticos ELF coredump metadata.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_CORE_ELF_METADATA_SCHEMA_VERSION_V1 (1)

typedef enum TicosCoreElfMetadataKey {
  kTicosCoreElfMetadataKey_SchemaVersion = 1,
  kTicosCoreElfMetadataKey_LinuxSdkVersion = 2,
  kTicosCoreElfMetadataKey_CapturedTime = 3,
  kTicosCoreElfMetadataKey_DeviceSerial = 4,
  kTicosCoreElfMetadataKey_HardwareVersion = 5,
  kTicosCoreElfMetadataKey_SoftwareType = 6,
  kTicosCoreElfMetadataKey_SoftwareVersion = 7,
} eTicosCoreElfMetadataKey;

typedef struct TicosCoreElfMetadata {
  const char *linux_sdk_version;
  uint32_t captured_time_epoch_s;
  const char *device_serial;
  const char *hardware_version;
  const char *software_type;
  const char *software_version;
} sTicosCoreElfMetadata;

size_t ticos_core_elf_metadata_note_calculate_size(const sTicosCoreElfMetadata *metadata);
bool ticos_core_elf_metadata_note_write(const sTicosCoreElfMetadata *metadata,
                                           uint8_t *note_buffer, size_t note_buffer_size);

#ifdef __cplusplus
}
#endif

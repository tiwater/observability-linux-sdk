//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ticosd device settings implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ticosd.h"

#define DEFAULT_BASE_URL "https://device.ticos.com"

#define INFO_BINARY "ticos-device-info"

/**
 * @brief Destroy the device settings object
 *
 * @param handle device settings object
 */
void ticosd_device_settings_destroy(sTicosdDeviceSettings *handle) {
  if (handle) {
    if (handle->device_id) {
      free(handle->device_id);
    }
    if (handle->hardware_version) {
      free(handle->hardware_version);
    }

    free(handle);
  }
}

/**
 * @brief Initialise the device settings object
 *
 * @return ticosd_device_settings_t* device settings object
 */
sTicosdDeviceSettings *ticosd_device_settings_init(void) {
  FILE *fd = popen(INFO_BINARY, "r");
  if (!fd) {
    fprintf(stderr, "device_settings:: Unable to execute '%s'\n", INFO_BINARY);
    return NULL;
  }

  sTicosdDeviceSettings *handle = calloc(sizeof(sTicosdDeviceSettings), 1);

  char line[1024];
  while (fgets(line, sizeof(line), fd)) {
    char *name = strtok(line, "=");
    char *val = strtok(NULL, "\n");

    if (strcmp(name, "TICOS_DEVICE_ID") == 0) {
      if (!val || strlen(val) == 0) {
        fprintf(stderr, "device_settings:: Invalid TICOS_DEVICE_ID setting\n");
        continue;
      }
      handle->device_id = strdup(val);
    } else if (strcmp(name, "TICOS_HARDWARE_VERSION") == 0) {
      if (!val || strlen(val) == 0) {
        fprintf(stderr, "device_settings:: Invalid TICOS_HARDWARE_VERSION setting\n");
        continue;
      }
      handle->hardware_version = strdup(val);
    } else {
      fprintf(stderr, "device_settings:: Unknown option in %s : '%s'\n", INFO_BINARY, name);
    }
  }

  pclose(fd);

  bool failed = false;
  if (!handle->device_id) {
    fprintf(stderr, "device_settings:: TICOS_DEVICE_ID not set in %s\n", INFO_BINARY);
    failed = true;
  }
  if (!handle->hardware_version) {
    fprintf(stderr, "device_settings:: TICOS_HARDWARE_VERSION not set in %s\n", INFO_BINARY);
    failed = true;
  }

  if (failed) {
    ticosd_device_settings_destroy(handle);
    return NULL;
  }

  return handle;
}

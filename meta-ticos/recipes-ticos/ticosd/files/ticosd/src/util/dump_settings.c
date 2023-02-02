//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Print the current configuration, runtime settings and build time settings to the console.

#include <stdio.h>
#include <string.h>

#include "ticos/util/config.h"
#include "ticos/util/device_settings.h"
#include "ticos/util/plugins.h"
#include "ticos/util/version.h"
#include "ticosd.h"

void ticosd_dump_settings(sTicosdDeviceSettings *settings, sTicosdConfig *config,
                             const char *config_file) {
  ticosd_config_dump_config(config, config_file);

  if (settings) {
    printf("Device configuration from ticos-device-info:\n");
    printf("  TICOS_DEVICE_ID=%s\n", settings->device_id);
    printf("  TICOS_HARDWARE_VERSION=%s\n", settings->hardware_version);
  } else {
    printf("Device configuration from ticos-device-info: IS NOT AVAILABLE.\n");
  }
  printf("\n");

  ticos_version_print_info();
  printf("\n");

  printf("Plugin enabled:\n");
  for (unsigned int i = 0; i < g_plugins_count; ++i) {
    if (g_plugins[i].name[0] != '\0') {
      printf("  %s\n", g_plugins[i].name);
    }
  }
  printf("\n");
}

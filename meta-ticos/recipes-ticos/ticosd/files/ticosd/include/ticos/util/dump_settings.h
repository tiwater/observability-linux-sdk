#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Print the current configuration, runtime settings and build time settings to the console.

#ifdef __cplusplus
extern "C" {
#endif

void ticosd_dump_settings(sTicosdDeviceSettings *settings, sTicosdConfig *config,
                             const char *config_file);

#ifdef __cplusplus
}
#endif

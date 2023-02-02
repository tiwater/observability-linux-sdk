//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ticosd device settings API definition

#ifndef __DEVICE_SETTINGS_H
#define __DEVICE_SETTINGS_H

#include "ticosd.h"

#ifdef __cplusplus
extern "C" {
#endif

sTicosdDeviceSettings *ticosd_device_settings_init(void);
void ticosd_device_settings_destroy(sTicosdDeviceSettings *handle);

#ifdef __cplusplus
}
#endif

#endif

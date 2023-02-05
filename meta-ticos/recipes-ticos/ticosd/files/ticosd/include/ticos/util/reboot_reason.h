#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!

#include <stdbool.h>

#include "ticos/core/reboot_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ticosd_is_reboot_reason_valid(ticosRebootReason reboot_reason);
const char *ticosd_reboot_reason_str(ticosRebootReason reboot_reason);

#ifdef __cplusplus
}
#endif

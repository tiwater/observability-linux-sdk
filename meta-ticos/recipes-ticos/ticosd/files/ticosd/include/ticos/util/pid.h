#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!

#define PID_FILE "/var/run/ticosd.pid"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ticosd_check_for_pid_file(void);
int ticosd_get_pid(void);

#ifdef __cplusplus
}
#endif

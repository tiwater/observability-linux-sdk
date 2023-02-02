#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ticosd systemd helper

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ticosd_restart_service_if_running(const char *service_name);
bool ticosd_kill_service(const char *service_name, int signal);

#ifdef __cplusplus
}
#endif

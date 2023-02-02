#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Disk utilities

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t ticosd_get_free_space(const char* path, bool privileged);
size_t ticosd_get_folder_size(const char* path);

#ifdef __cplusplus
}
#endif

#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

int ticos_set_runtime_bool_and_reload(sTicosdConfig *config, const char *config_key,
                                         const char *description, bool value);

#ifdef __cplusplus
}
#endif

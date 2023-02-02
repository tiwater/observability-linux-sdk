#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! String utilities.

#include "ticos/core/compiler.h"

TICOS_PRINTF_LIKE_FUNC(2, 3)
int ticos_asprintf(char **restrict strp, const char *fmt, ...);

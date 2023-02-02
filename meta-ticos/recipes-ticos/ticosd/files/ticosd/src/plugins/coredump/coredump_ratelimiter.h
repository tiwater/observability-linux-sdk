//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Rate limiting of coredumps.

#pragma once

#include "ticos/util/rate_limiter.h"
#include "ticosd.h"

#ifdef __cplusplus
extern "C" {
#endif

sTicosdRateLimiter *coredump_create_rate_limiter(sTicosd *ticosd);

#ifdef __cplusplus
}
#endif

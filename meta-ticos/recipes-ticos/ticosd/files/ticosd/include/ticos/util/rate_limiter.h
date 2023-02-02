#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Rate limiter library functions

#include <stdbool.h>

#include "ticosd.h"

typedef struct TicosdRateLimiter sTicosdRateLimiter;

#ifdef __cplusplus
extern "C" {
#endif

sTicosdRateLimiter *ticosd_rate_limiter_init(sTicosd *ticosd, const int count,
                                                   const int duration, const char *filename);
bool ticosd_rate_limiter_check_event(sTicosdRateLimiter *handle);
void ticosd_rate_limiter_destroy(sTicosdRateLimiter *handle);

#ifdef __cplusplus
}
#endif

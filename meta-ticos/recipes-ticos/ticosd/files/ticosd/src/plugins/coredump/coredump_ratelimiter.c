//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Rate limiting of coredumps.

#include "coredump_ratelimiter.h"

#define RATE_LIMIT_FILENAME "coredump_rate_limit"

/**
 * @brief Create a new coredump rate limiter.
 *
 * @param ticosd Main ticosd handle
 * @return sTicosdRateLimiter an initialized rate limiter
 */
sTicosdRateLimiter *coredump_create_rate_limiter(sTicosd *ticosd) {
  //! Initialise the corefile rate limiter, errors here aren't critical
  int rate_limit_count = 0;
  int rate_limit_duration_seconds = 0;
  if (!ticosd_is_dev_mode(ticosd)) {
    ticosd_get_integer(ticosd, "coredump_plugin", "rate_limit_count", &rate_limit_count);
    ticosd_get_integer(ticosd, "coredump_plugin", "rate_limit_duration_seconds",
                          &rate_limit_duration_seconds);
  }

  return ticosd_rate_limiter_init(ticosd, rate_limit_count, rate_limit_duration_seconds,
                                     RATE_LIMIT_FILENAME);
}

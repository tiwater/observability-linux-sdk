//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Unit tests for coredump_ratelimiter.c
//!

#include "coredump/coredump_ratelimiter.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <stdlib.h>
#include <string.h>

TEST_GROUP(TestCoreDumpRateLimiterGroup){};

extern "C" {

typedef struct TicosdRateLimiter {
  int count = -1;
  int duration = -1;
} sTicosdRateLimiter;

sTicosdRateLimiter *ticosd_rate_limiter_init(sTicosd *ticosd, const int count,
                                                   const int duration, const char *filename) {
  sTicosdRateLimiter *r = (sTicosdRateLimiter *)calloc(sizeof(sTicosdRateLimiter), 1);
  r->count = count;
  r->duration = duration;

  return r;
}

bool ticosd_get_integer(sTicosd *ticosd, const char *parent_key, const char *key,
                           int *val) {
  return mock()
    .actualCall("ticosd_get_integer")
    .withStringParameter("parent_key", parent_key)
    .withStringParameter("key", key)
    .withOutputParameter("val", val)
    .returnBoolValue();
}

bool ticosd_is_dev_mode(sTicosd *ticosd) {
  return mock().actualCall("ticosd_is_dev_mode").returnBoolValue();
}
}

/**
 * @brief When ticosd is not in dev mode
 *
 */
TEST(TestCoreDumpRateLimiterGroup, NormalMode) {
  int rate_limit_count = 42;
  int rate_limit_duration = 60;

  mock().expectOneCall("ticosd_is_dev_mode").andReturnValue(false);
  mock()
    .expectOneCall("ticosd_get_integer")
    .withStringParameter("parent_key", "coredump_plugin")
    .withStringParameter("key", "rate_limit_count")
    .withOutputParameterReturning("val", &rate_limit_count, sizeof(rate_limit_count))
    .andReturnValue(true);
  mock()
    .expectOneCall("ticosd_get_integer")
    .withStringParameter("parent_key", "coredump_plugin")
    .withStringParameter("key", "rate_limit_duration_seconds")
    .withOutputParameterReturning("val", &rate_limit_duration, sizeof(rate_limit_duration))
    .andReturnValue(true);

  auto r = coredump_create_rate_limiter(NULL);

  mock().checkExpectations();
  CHECK(r->count == rate_limit_count);
  CHECK(r->duration == rate_limit_duration);

  mock().clear();
  free(r);
}

/**
 * @brief When ticosd is in dev mode
 *
 */
TEST(TestCoreDumpRateLimiterGroup, DevMode) {
  mock().expectOneCall("ticosd_is_dev_mode").andReturnValue(true);

  auto r = coredump_create_rate_limiter(NULL);

  mock().checkExpectations();
  mock().clear();

  CHECK(r->count == 0);
  CHECK(r->duration == 0);
  free(r);
}

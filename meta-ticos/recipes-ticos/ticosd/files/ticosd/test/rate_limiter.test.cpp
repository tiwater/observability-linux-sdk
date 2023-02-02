//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Unit tests for rate_limiter.c
//!

#include "ticos/util/rate_limiter.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ticosd.h"

static sTicosd *g_stub_ticosd = (sTicosd *)~0;

extern "C" {
time_t *ticosd_rate_limiter_get_history(sTicosdRateLimiter *handle);
}

char *ticosd_generate_rw_filename(sTicosd *ticosd, const char *filename) {
  const char *path = mock()
                       .actualCall("ticosd_generate_rw_filename")
                       .withPointerParameter("ticosd", ticosd)
                       .withStringParameter("filename", filename)
                       .returnStringValue();
  return strdup(path);  //! original returns malloc'd string
}

TEST_BASE(TicosdRateLimiterUtest) {
  char tmp_dir[32] = {0};
  char tmp_reboot_file[64] = {0};
  struct timeval tv = {0};

  void setup() override {
    strcpy(tmp_dir, "/tmp/ticosd.XXXXXX");
    mkdtemp(tmp_dir);
    sprintf(tmp_reboot_file, "%s/ratelimit", tmp_dir);
  }

  void teardown() override {
    unlink(tmp_reboot_file);
    rmdir(tmp_dir);
    mock().checkExpectations();
    mock().clear();
  }

  void expect_generate_ratelimit_filename_call(const char *path) {
    mock()
      .expectOneCall("ticosd_generate_rw_filename")
      .withPointerParameter("ticosd", g_stub_ticosd)
      .withStringParameter("filename", "ratelimit")
      .andReturnValue(tmp_reboot_file);
  }

  void write_ratelimit_file(const char *val) {
    std::ofstream fd(tmp_reboot_file);
    fd << val;
  }

  char *read_ratelimit_file() {
    std::ifstream fd(tmp_reboot_file);
    std::stringstream buf;
    buf << fd.rdbuf();
    return strdup(buf.str().c_str());
  }
};

TEST_GROUP_BASE(TestGroup_Init, TicosdRateLimiterUtest){};

TEST(TestGroup_Init, InitFailures) {
  CHECK(!ticosd_rate_limiter_init(NULL, 5, 3600, NULL));              //! ticosd is NULL
  CHECK(!ticosd_rate_limiter_init(g_stub_ticosd, 0, 3600, NULL));  //! count is 0
  CHECK(!ticosd_rate_limiter_init(g_stub_ticosd, 5, 0, NULL));     //! duration is 0
}

TEST(TestGroup_Init, InitSuccessNoHistoryFile) {
  sTicosdRateLimiter *handle = ticosd_rate_limiter_init(g_stub_ticosd, 5, 3600, NULL);
  CHECK(handle);

  time_t *history = ticosd_rate_limiter_get_history(handle);
  for (int i = 0; i < 5; ++i) {
    CHECK_EQUAL(0, history[i]);
  }

  ticosd_rate_limiter_destroy(handle);
}

TEST(TestGroup_Init, InitSuccessWithEmptyHistoryFile) {
  expect_generate_ratelimit_filename_call("ratelimit");
  sTicosdRateLimiter *handle =
    ticosd_rate_limiter_init(g_stub_ticosd, 5, 3600, "ratelimit");
  CHECK(handle);

  time_t *history = ticosd_rate_limiter_get_history(handle);
  for (int i = 0; i < 5; ++i) {
    CHECK_EQUAL(0, history[i]);
  }

  ticosd_rate_limiter_destroy(handle);
}

TEST(TestGroup_Init, InitSuccessWithPopulatedHistoryFile) {
  write_ratelimit_file("500 400 300 200 100 ");

  expect_generate_ratelimit_filename_call("ratelimit");
  sTicosdRateLimiter *handle =
    ticosd_rate_limiter_init(g_stub_ticosd, 5, 3600, "ratelimit");

  time_t *history = ticosd_rate_limiter_get_history(handle);
  CHECK_EQUAL(500, history[0]);
  CHECK_EQUAL(400, history[1]);
  CHECK_EQUAL(300, history[2]);
  CHECK_EQUAL(200, history[3]);
  CHECK_EQUAL(100, history[4]);

  ticosd_rate_limiter_destroy(handle);
}

TEST_GROUP_BASE(TestGroup_CheckEvent, TicosdRateLimiterUtest){};

TEST(TestGroup_CheckEvent, EventSuccessNoLimiting) {
  CHECK_EQUAL(true, ticosd_rate_limiter_check_event(NULL));
}

TEST(TestGroup_CheckEvent, EventSuccessHistoryUpdated) {
  write_ratelimit_file("500 400 300 200 100 ");

  expect_generate_ratelimit_filename_call("ratelimit");
  sTicosdRateLimiter *handle =
    ticosd_rate_limiter_init(g_stub_ticosd, 5, 3600, "ratelimit");

  CHECK_EQUAL(true, ticosd_rate_limiter_check_event(handle));

  time_t *history = ticosd_rate_limiter_get_history(handle);
  CHECK(500 != history[0]);
  CHECK_EQUAL(500, history[1]);
  CHECK_EQUAL(400, history[2]);
  CHECK_EQUAL(300, history[3]);
  CHECK_EQUAL(200, history[4]);

  char expected_file[256] = {'\0'};
  snprintf(expected_file, sizeof(expected_file), "%ld 500 400 300 200 ", history[0]);

  char *actual_file = read_ratelimit_file();

  STRCMP_EQUAL(expected_file, actual_file);
  free(actual_file);

  ticosd_rate_limiter_destroy(handle);
}

TEST(TestGroup_CheckEvent, EventLimitReached) {
  sTicosdRateLimiter *handle = ticosd_rate_limiter_init(g_stub_ticosd, 5, 3600, NULL);

  struct timeval now;
  gettimeofday(&now, NULL);

  time_t *history = ticosd_rate_limiter_get_history(handle);
  //! Oldest record is newer than /duration/
  history[4] = now.tv_sec - 3600 + 2;

  CHECK_EQUAL(false, ticosd_rate_limiter_check_event(handle));

  ticosd_rate_limiter_destroy(handle);
}

TEST(TestGroup_CheckEvent, EventLimitNotReached) {
  sTicosdRateLimiter *handle = ticosd_rate_limiter_init(g_stub_ticosd, 5, 3600, NULL);

  struct timeval now;
  gettimeofday(&now, NULL);

  time_t *history = ticosd_rate_limiter_get_history(handle);
  //! Oldest record is older than /duration/
  history[4] = now.tv_sec - 3600 - 2;

  CHECK_EQUAL(true, ticosd_rate_limiter_check_event(handle));

  ticosd_rate_limiter_destroy(handle);
}

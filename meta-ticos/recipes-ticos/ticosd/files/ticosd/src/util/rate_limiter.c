//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Rate limiter library functions

#include "ticos/util/rate_limiter.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ticosd.h"

struct TicosdRateLimiter {
  int count;
  int duration;
  time_t *history;
  char *file;
};

bool ticosd_rate_limiter_check_event(sTicosdRateLimiter *handle) {
  if (!handle) {
    //! Rate limiting disabled
    return true;
  }

  struct timeval now;
  gettimeofday(&now, NULL);

  if (handle->history[handle->count - 1] + handle->duration > now.tv_sec) {
    fprintf(stderr, "rate_limiter:: Rejecting event, rate limit reached\n");
    return false;
  }

  //! Shuffle right, adding new time into [0]
  for (int i = handle->count - 2; i >= 0; --i) {
    handle->history[i + 1] = handle->history[i];
  }
  handle->history[0] = now.tv_sec;

  if (handle->file) {
    FILE *fd = fopen(handle->file, "w");
    if (!fd) {
      //! Failed to write rate_limit file, but return true to proceed with rest of processing
      fprintf(stderr, "rate_limiter:: Failed to open rate_limit file\n");
      return true;
    }

    for (int i = 0; i < handle->count; ++i) {
      fprintf(fd, "%ld ", handle->history[i]);
    }

    fclose(fd);
  }

  return true;
}

void ticosd_rate_limiter_destroy(sTicosdRateLimiter *handle) {
  if (handle) {
    free(handle->file);
    free(handle->history);
    free(handle);
  }
}

sTicosdRateLimiter *ticosd_rate_limiter_init(sTicosd *ticosd, const int count,
                                                   const int duration, const char *filename) {
  if (count == 0 || duration == 0 || !ticosd) {
    return NULL;
  }

  sTicosdRateLimiter *handle;
  if (!(handle = calloc(sizeof(sTicosdRateLimiter), 1))) {
    fprintf(stderr, "rate_limiter:: Failed to allocate handle\n");
    goto cleanup;
  }

  handle->count = count;
  handle->duration = duration;

  if (!(handle->history = calloc(sizeof(time_t), handle->count))) {
    fprintf(stderr, "rate_limiter:: Failed to allocate history array\n");
    goto cleanup;
  }

  if (filename) {
    if (!(handle->file = ticosd_generate_rw_filename(ticosd, filename))) {
      fprintf(stderr, "rate_limiter:: Failed to generate history filename\n");
      goto cleanup;
    }

    FILE *fd = fopen(handle->file, "r");
    if (!fd) {
      if (errno != ENOENT) {
        //! File exists, but we can't open it
        fprintf(stderr, "rate_limiter:: Failed to open history file\n");
        goto cleanup;
      }
    } else {
      for (int i = 0; i < handle->count; ++i) {
        if (fscanf(fd, "%ld ", &handle->history[i]) != 1) {
          handle->history[i] = 0;
          break;
        }
      }

      fclose(fd);
    }
  }

  return handle;

cleanup:

  free(handle->file);
  free(handle->history);
  free(handle);
  return NULL;
}

#ifdef TICOS_UNITTEST

time_t *ticosd_rate_limiter_get_history(sTicosdRateLimiter *handle) {
  return handle->history;
}

#endif

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! String utilities.

#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

int ticos_asprintf(char **restrict strp, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  int len = vsnprintf(NULL, 0, fmt, args);
  *strp = malloc(len + 1);
  if (*strp == NULL) {
    fprintf(stderr, "util_string:: Failed to allocate buffer\n");
    va_end(args);
    return -1;
  }
  va_end(args);

  va_start(args, fmt);
  len = vsprintf(*strp, fmt, args);
  va_end(args);

  return len;
}

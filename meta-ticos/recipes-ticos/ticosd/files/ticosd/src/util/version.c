//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Version information for ticos Linux SDK.
#include <stdio.h>

#ifndef VERSION
  #define VERSION dev
#endif
#ifndef GITCOMMIT
  #define GITCOMMIT unknown
#endif

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

const char ticosd_sdk_version[] = STRINGIZE_VALUE_OF(VERSION);

/**
 * @brief Displays SDK version information
 *
 */
void ticos_version_print_info(void) {
  printf("VERSION=%s\n", STRINGIZE_VALUE_OF(VERSION));
  printf("GIT COMMIT=%s\n", STRINGIZE_VALUE_OF(GITCOMMIT));
}

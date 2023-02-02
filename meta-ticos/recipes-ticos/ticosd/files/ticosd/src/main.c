//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Main file for ticos linux SDK.
//!
//! @details
//! We build one binary on disk and create two links to it (ticosd and ticosctl).
//! This approach is inspired by the busybox project.

#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "ticosctl/ticosctl.h"
#include "ticosd.h"

int main(int argc, char **argv) {
  char *cmd_name = basename(argv[0]);
  if (strcmp(cmd_name, "ticosd") == 0) {
    return ticosd_main(argc, argv);
  } else {
    return ticosctl_main(argc, argv);
  }
}

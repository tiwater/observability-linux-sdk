#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!

#include <stdbool.h>

#include "ticosctl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ErrorType {
  eErrorTypeSegFault,
  eErrorTypeFPException,
} eErrorType;

/**
 * Creates a new process and force it to crash with error e.
 *
 * @return 0 on success.
 */
void ticos_trigger_crash(eErrorType e);

#ifdef __cplusplus
}
#endif

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Network POST & GET API wrapper around libCURL
//!

#ifndef __TICOS_NETWORK_H
#define __TICOS_NETWORK_H

#include <stdbool.h>
#include <stddef.h>

#include "ticosd.h"

typedef enum TicosdHttpMethod {
  kTicosdHttpMethod_POST,
  kTicosdHttpMethod_PATCH
} ticosdHttpMethod;

typedef enum TicosdNetworkResult {
  /**
   * The network operation was successful.
   */
  kTicosdNetworkResult_OK,
  /**
   * The network operation was not successful, but retrying later is sensible because the error is
   * likely to be transient.
   */
  kTicosdNetworkResult_ErrorRetryLater,
  /**
   * The network operation was not successful and retrying is not sensible because the error is
   * not transient.
   */
  kTicosdNetworkResult_ErrorNoRetry,
} ticosdNetworkResult;

typedef struct TicosdNetwork sTicosdNetwork;

sTicosdNetwork *ticosd_network_init(sTicosd *ticosd);
void ticosd_network_destroy(sTicosdNetwork *handle);
ticosdNetworkResult ticosd_network_post(sTicosdNetwork *handle, const char *endpoint,
                                               ticosdHttpMethod method, const char *payload,
                                               char **data, size_t *len);

ticosdNetworkResult ticosd_network_file_upload(sTicosdNetwork *handle,
                                                      const char *commit_endpoint,
                                                      const char *payload, bool is_gzipped);

#endif

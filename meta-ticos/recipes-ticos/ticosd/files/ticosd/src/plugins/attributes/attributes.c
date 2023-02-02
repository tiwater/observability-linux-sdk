//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ticos attributes plugin implementation

#include <errno.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <systemd/sd-bus.h>
#include <unistd.h>

#include "ticos/core/math.h"
#include "ticos/util/ipc.h"
#include "ticos/util/string.h"
#include "ticosd.h"

struct TicosdPlugin {
  sTicosd *ticosd;
};

static sTicosdTxData *prv_build_queue_entry(sTicosAttributesIPC *msg,
                                               uint32_t *payload_size) {
  size_t json_len = strlen(msg->json);
  *payload_size = sizeof(time_t) + json_len + 1;

  sTicosdTxDataAttributes *data;
  if (!(data = malloc(sizeof(sTicosdTxDataAttributes) + *payload_size))) {
    fprintf(stderr, "network:: Failed to create upload_request buffer\n");
    return NULL;
  }

  data->type = kTicosdTxDataType_Attributes;
  data->timestamp = msg->timestamp;
  strcpy((char *)data->json, msg->json);

  return (sTicosdTxData *)data;
}

/**
 * Build a queue entry for ticosd.
 */
static bool prv_msg_handler(sTicosdPlugin *handle, struct msghdr *msghdr, size_t received_size) {
  int ret = EXIT_SUCCESS;

  sTicosAttributesIPC *msg = msghdr->msg_iov[0].iov_base;

  // Transform JSON Array into a Queue message
  uint32_t len = 0;
  struct TicosdTxData *data = prv_build_queue_entry(msg, &len);

  if (!ticosd_txdata(handle->ticosd, data, len)) {
    ret = EXIT_FAILURE;
    goto cleanup;
  }

cleanup:
  free(data);
  return ret == EXIT_SUCCESS;
}

static sTicosdPluginCallbackFns s_fns = {.plugin_ipc_msg_handler = prv_msg_handler};

/**
 * @brief Initialises attributes plugin
 *
 * @param ticosd Main ticosd handle
 * @return callbackFunctions_t Plugin function table
 */
bool ticosd_attributes_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns) {
  sTicosdPlugin *handle = calloc(sizeof(sTicosdPlugin), 1);
  if (!handle) {
    fprintf(stderr, "attributes:: Failed to allocate plugin handle\n");
    return false;
  }

  handle->ticosd = ticosd;
  *fns = &s_fns;
  (*fns)->handle = handle;

  return true;
}

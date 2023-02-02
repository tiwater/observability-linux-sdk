#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Interact via IPC with ticosd.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TICOSD_IPC_SOCKET_PATH "/tmp/ticos-ipc.sock"

/**
 * Send a SIGUSR1 signal to ticosd to immediately process the queue.
 */
bool ticosd_send_flush_queue_signal(void);

/**
 * Send an IPC message to ticosd.
 *
 * The first bytes of the message should be the ipc_plugin_name of the plugin to process the
 * message.
 */
bool ticosd_ipc_sendmsg(uint8_t *msg, size_t len);

typedef struct TicosAttributesIPC {
  char name[11] /*"ATTRIBUTES\0" */;
  time_t timestamp;
  char json[];
} sTicosAttributesIPC;

#ifdef __cplusplus
}
#endif

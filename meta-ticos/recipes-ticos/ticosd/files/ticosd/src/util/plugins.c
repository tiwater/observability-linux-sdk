//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!

#include "ticos/util/plugins.h"

#include <stdio.h>
#include <string.h>

#include "ticos/core/math.h"

sTicosdPluginDef g_plugins[] = {
  {.name = "attributes", .init = ticosd_attributes_init, .ipc_name = "ATTRIBUTES"},
#ifdef PLUGIN_REBOOT
  {.name = "reboot", .init = ticosd_reboot_init},
#endif
#ifdef PLUGIN_SWUPDATE
  {.name = "swupdate", .init = ticosd_swupdate_init},
#endif
#ifdef PLUGIN_COLLECTD
  {.name = "collectd", .init = ticosd_collectd_init, .ipc_name = PLUGIN_COLLECTD_IPC_NAME},
#endif
#ifdef PLUGIN_COREDUMP
  {.name = "coredump", .init = ticosd_coredump_init, .ipc_name = "CORE"},
#endif
};

const unsigned long int g_plugins_count = TICOS_ARRAY_SIZE(g_plugins);

void ticosd_load_plugins(sTicosd *handle) {
  for (unsigned int i = 0; i < g_plugins_count; ++i) {
    if (!g_plugins[i].init(handle, &g_plugins[i].fns)) {
      fprintf(stderr, "ticosd:: Failed to initialize %s plugin, destroying.\n",
              g_plugins[i].name);
      g_plugins[i].fns = NULL;
    }
  }
}

void ticosd_destroy_plugins(void) {
  for (unsigned int i = 0; i < g_plugins_count; ++i) {
    if (g_plugins[i].fns != NULL && g_plugins[i].fns->plugin_destroy) {
      g_plugins[i].fns->plugin_destroy(g_plugins[i].fns->handle);
    }
  }
}

bool ticosd_plugins_process_ipc(struct msghdr *msg, size_t received_size) {
  for (unsigned int i = 0; i < g_plugins_count; ++i) {
    if (g_plugins[i].ipc_name[0] == '\0' || !g_plugins[i].fns ||
        !g_plugins[i].fns->plugin_ipc_msg_handler) {
      // Plugin doesn't process IPC messages
      continue;
    }

    if (received_size <= strlen(g_plugins[i].ipc_name) ||
        strcmp(g_plugins[i].ipc_name, msg->msg_iov[0].iov_base) != 0) {
      // Plugin doesn't match IPC signature
      continue;
    }

    if (!g_plugins[i].fns->plugin_ipc_msg_handler(g_plugins[i].fns->handle, msg, received_size)) {
      fprintf(stderr, "ticosd:: Plugin %s failed to process IPC message.\n", g_plugins[i].name);
    }
    return true;
  }
  return false;
}

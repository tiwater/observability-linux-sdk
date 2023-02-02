#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Definition for plugins entrypoints.

#include "ticosd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  ticosd_plugin_init init;
  sTicosdPluginCallbackFns *fns;
  const char name[32];
  const char ipc_name[32];
} sTicosdPluginDef;

#define PLUGIN_ATTRIBUTES_IPC_NAME "ATTRIBUTES"
bool ticosd_attributes_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns);

#ifdef PLUGIN_REBOOT
bool ticosd_reboot_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns);
void ticosd_reboot_data_collection_enabled(sTicosd *ticosd, bool data_collection_enabled);
#endif
#ifdef PLUGIN_SWUPDATE
bool ticosd_swupdate_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns);
#endif
#ifdef PLUGIN_COLLECTD
  #define PLUGIN_COLLECTD_IPC_NAME "COLLECTD"
bool ticosd_collectd_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns);
#endif
#ifdef PLUGIN_COREDUMP
bool ticosd_coredump_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns);
#endif

/**
 * @brief List of all enabled plugins.
 */
extern sTicosdPluginDef g_plugins[];

/**
 * @brief Count of enabled plugins.
 */
extern const unsigned long int g_plugins_count;

/**
 * @brief Call the init function of all defined plugins
 *
 * @param handle Main ticosd handle
 */
void ticosd_load_plugins(sTicosd *handle);

/**
 * @brief Call the destroy function of all defined g_plugins
 */
void ticosd_destroy_plugins(void);

/**
 * @brief Search for a plugin to process this IPC message and delegate processing.
 */
bool ticosd_plugins_process_ipc(struct msghdr *msg, size_t received_size);

#ifdef __cplusplus
}
#endif

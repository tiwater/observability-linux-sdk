//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! config init and handling definition

#ifndef __TICOS_config_H
#define __TICOS_config_H

#include <ticosd.h>
#include <stdbool.h>

#define CONFIG_FILE "/etc/ticosd.conf"
#define CONFIG_KEY_DEV_MODE "enable_dev_mode"
#define CONFIG_KEY_DATA_COLLECTION "enable_data_collection"

typedef struct TicosdConfig sTicosdConfig;

sTicosdConfig *ticosd_config_init(sTicosd *ticosd, const char *file);
void ticosd_config_destroy(sTicosdConfig *handle);
void ticosd_config_dump_config(sTicosdConfig *handle, const char *file);

void ticosd_config_set_string(sTicosdConfig *handle, const char *parent_key, const char *key,
                                 const char *val);
void ticosd_config_set_integer(sTicosdConfig *handle, const char *parent_key, const char *key,
                                  const int val);
void ticosd_config_set_boolean(sTicosdConfig *handle, const char *parent_key, const char *key,
                                  const bool val);

bool ticosd_config_get_string(sTicosdConfig *handle, const char *parent_key, const char *key,
                                 const char **val);
bool ticosd_config_get_integer(sTicosdConfig *handle, const char *parent_key, const char *key,
                                  int *val);
bool ticosd_config_get_boolean(sTicosdConfig *handle, const char *parent_key, const char *key,
                                  bool *val);
bool ticosd_config_get_objects(sTicosdConfig *handle, const char *parent_key,
                                  sTicosdConfigObject **objects, int *len);
char *ticosd_config_generate_rw_filename(sTicosdConfig *handle, const char *filename);

#endif

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ticosd plugin API definition

#ifndef __TICOS_H
#define __TICOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

typedef struct Ticosd sTicosd;
typedef struct TicosdPlugin sTicosdPlugin;

typedef bool (*ticosd_plugin_reload)(sTicosdPlugin *plugin);
typedef void (*ticosd_plugin_destroy)(sTicosdPlugin *plugin);
typedef bool (*ticosd_plugin_ipc_msg_handler)(sTicosdPlugin *handle, struct msghdr *msg,
                                                 size_t received_size);

typedef enum {
  kTicosdConfigTypeUnknown,
  kTicosdConfigTypeBoolean,
  kTicosdConfigTypeInteger,
  kTicosdConfigTypeString,
  kTicosdConfigTypeObject
} ticosdConfigType;

typedef struct {
  sTicosdPlugin *handle;
  ticosd_plugin_reload plugin_reload;
  ticosd_plugin_destroy plugin_destroy;
  ticosd_plugin_ipc_msg_handler plugin_ipc_msg_handler;
} sTicosdPluginCallbackFns;

typedef struct {
  char *device_id;
  char *hardware_version;
} sTicosdDeviceSettings;

typedef struct {
  const char *key;
  ticosdConfigType type;
  union {
    bool b;
    int d;
    const char *s;
  } value;
} sTicosdConfigObject;

typedef bool (*ticosd_plugin_init)(sTicosd *ticosd, sTicosdPluginCallbackFns **fns);

typedef enum {
  kTicosdTxDataType_RebootEvent = 'R',
  kTicosdTxDataType_CoreUpload = 'C',
  kTicosdTxDataType_CoreUploadWithGzip = 'c',
  kTicosdTxDataType_Attributes = 'A',
} ticosdTxDataType;

typedef struct __attribute__((__packed__)) TicosdTxData {
  uint8_t type;  // ticosdTxDataType
  uint8_t payload[];
} sTicosdTxData;

typedef struct __attribute__((__packed__)) TicosdTxDataAttributes {
  uint8_t type;  // ticosdTxDataType
  time_t timestamp;
  char json[];
} sTicosdTxDataAttributes;

int ticosd_main(int argc, char *argv[]);

bool ticosd_txdata(sTicosd *ticosd, const sTicosdTxData *data, uint32_t payload_size);

bool ticosd_get_boolean(sTicosd *ticosd, const char *parent_key, const char *key,
                           bool *val);
bool ticosd_get_integer(sTicosd *ticosd, const char *parent_key, const char *key,
                           int *val);
bool ticosd_get_string(sTicosd *ticosd, const char *parent_key, const char *key,
                          const char **val);
bool ticosd_get_objects(sTicosd *ticosd, const char *parent_key,
                           sTicosdConfigObject **objects, int *len);

const sTicosdDeviceSettings *ticosd_get_device_settings(sTicosd *ticosd);

char *ticosd_generate_rw_filename(sTicosd *ticosd, const char *filename);

bool ticosd_is_dev_mode(sTicosd *ticosd);

#ifdef __cplusplus
}
#endif

#endif

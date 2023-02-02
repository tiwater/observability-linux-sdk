//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ticos swupdate plugin implementation

#include <libconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>

#include "ticos/util/systemd.h"
#include "ticosd.h"

#define DEFAULT_INPUT_FILE "/etc/swupdate.cfg"
#define DEFAULT_OUTPUT_FILE "/tmp/swupdate.cfg"

#define DEFAULT_SURICATTA_TENANT "default"

#define HAWKBIT_PATH "/api/v0/hawkbit"

struct TicosdPlugin {
  sTicosd *ticosd;
};

/**
 * @brief Add 'global' section to config
 *
 * @param handle swupdate plugin handle
 * @param config config object to build into
 * @return true Successfully added global options to config
 * @return false Failed to add
 */
static bool prv_swupdate_add_globals(config_t *config) {
  if (!config_lookup(config, "globals")) {
    if (!config_setting_add(config_root_setting(config), "globals", CONFIG_TYPE_GROUP)) {
      fprintf(stderr, "swupdate:: Failed to add globals setting group\n");
      return false;
    }
  }
  return true;
}

/**
 * @brief Add 'suricatta' section to config
 *
 * @param handle swupdate plugin handle
 * @param config config object to build into
 * @return true Successfully added suricatta options to config
 * @return false Failed to add
 */
static bool prv_swupdate_add_suricatta(sTicosdPlugin *handle, config_t *config) {
  config_setting_t *suricatta = config_lookup(config, "suricatta");
  if (!suricatta) {
    if (!(suricatta =
            config_setting_add(config_root_setting(config), "suricatta", CONFIG_TYPE_GROUP))) {
      fprintf(stderr, "swupdate:: Failed to add suricatta group\n");
      return false;
    }
  }

  const sTicosdDeviceSettings *settings = ticosd_get_device_settings(handle->ticosd);

  const char *base_url;
  if (!ticosd_get_string(handle->ticosd, "", "base_url", &base_url)) {
    return false;
  }

  char *url = malloc(strlen(HAWKBIT_PATH) + strlen(base_url) + 1);
  strcpy(url, base_url);
  strcat(url, HAWKBIT_PATH);

  config_setting_t *element;
  config_setting_remove(suricatta, "url");
  if (!(element = config_setting_add(suricatta, "url", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, url)) {
    fprintf(stderr, "swupdate:: Failed to add suricatta:url\n");
    free(url);
    return false;
  }

  free(url);

  config_setting_remove(suricatta, "id");
  if (!(element = config_setting_add(suricatta, "id", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, settings->device_id)) {
    fprintf(stderr, "swupdate:: Failed to add suricatta:id\n");
    return false;
  }

  config_setting_remove(suricatta, "tenant");
  if (!(element = config_setting_add(suricatta, "tenant", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, DEFAULT_SURICATTA_TENANT)) {
    fprintf(stderr, "swupdate:: Failed to add suricatta:tenant\n");
    return false;
  }

  const char *project_key;
  if (!ticosd_get_string(handle->ticosd, "", "project_key", &project_key)) {
    return false;
  }

  config_setting_remove(suricatta, "gatewaytoken");
  if (!(element = config_setting_add(suricatta, "gatewaytoken", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, project_key)) {
    fprintf(stderr, "plugin_swupdate:: Failed to add suricatta:id\n");
    return false;
  }

  return true;
}

/**
 * @brief Add 'identify' section to config
 *
 * @param handle swupdate plugin handle
 * @param config config object to build into
 * @return true Successfully added identify options to config
 * @return false Failed to add
 */
static bool prv_swupdate_add_identify(sTicosdPlugin *handle, config_t *config) {
  config_setting_t *identify;
  const sTicosdDeviceSettings *settings = ticosd_get_device_settings(handle->ticosd);

  config_setting_remove(config_root_setting(config), "identify");
  if (!(identify = config_setting_add(config_root_setting(config), "identify", CONFIG_TYPE_LIST))) {
    fprintf(stderr, "swupdate:: Failed to add identify list\n");
    return false;
  }

  const char *software_version;
  if (!ticosd_get_string(handle->ticosd, "", "software_version", &software_version)) {
    return false;
  }
  const char *software_type;
  if (!ticosd_get_string(handle->ticosd, "", "software_type", &software_type)) {
    return false;
  }

  config_setting_t *setting;
  config_setting_t *element;
  if (!(setting = config_setting_add(identify, NULL, CONFIG_TYPE_GROUP))) {
    fprintf(stderr, "swupdate:: Failed to add identify current_version\n");
    return false;
  }
  if (!(element = config_setting_add(setting, "name", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, "ticos__current_version")) {
    fprintf(stderr, "swupdate:: Failed to add identify current_version\n");
    return false;
  }
  if (!(element = config_setting_add(setting, "value", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, software_version)) {
    fprintf(stderr, "swupdate:: Failed to add identify current_version\n");
    return false;
  }

  if (!(setting = config_setting_add(identify, NULL, CONFIG_TYPE_GROUP))) {
    fprintf(stderr, "swupdate:: Failed to add identify hardware_version\n");
    return false;
  }
  if (!(element = config_setting_add(setting, "name", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, "ticos__hardware_version")) {
    fprintf(stderr, "swupdate:: Failed to add identify hardware_version\n");
    return false;
  }
  if (!(element = config_setting_add(setting, "value", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, settings->hardware_version)) {
    fprintf(stderr, "swupdate:: Failed to add identify hardware_version\n");
    return false;
  }

  if (!(setting = config_setting_add(identify, NULL, CONFIG_TYPE_GROUP))) {
    fprintf(stderr, "swupdate:: Failed to add identify software_type\n");
    return false;
  }
  if (!(element = config_setting_add(setting, "name", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, "ticos__software_type")) {
    fprintf(stderr, "swupdate:: Failed to add identify software_type\n");
    return false;
  }
  if (!(element = config_setting_add(setting, "value", CONFIG_TYPE_STRING)) ||
      !config_setting_set_string(element, software_type)) {
    fprintf(stderr, "swupdate:: Failed to add identify software_type\n");
    return false;
  }

  return true;
}

/**
 * @brief Add ticosd runtime.cfg overrides to config
 *
 * @param handle swupdate plugin handle
 * @param config config object to build into
 * @return true Successfully added runtime.cfg options
 * @return false Failed to add
 */
static bool prv_swupdate_add_runtime() { return true; }

/**
 * @brief Generate new swupdate.cfg file from config
 *
 * @param handle swupdate plugin handle
 * @return true Successfully generated new config
 * @return false Failed to generate
 */
static bool prv_swupdate_generate_config(sTicosdPlugin *handle) {
  config_t config;

  const char *input_file;
  if (!ticosd_get_string(handle->ticosd, "swupdate_plugin", "input_file", &input_file)) {
    input_file = DEFAULT_INPUT_FILE;
  }

  config_init(&config);
  if (!config_read_file(&config, input_file)) {
    fprintf(stderr,
            "swupdate:: Failed to read '%s', proceeding "
            "with defaults\n",
            input_file);
  }

  if (!prv_swupdate_add_globals(&config)) {
    fprintf(stderr, "swupdate:: Failed to add global options to config\n");
    config_destroy(&config);
    return false;
  }
  if (!prv_swupdate_add_suricatta(handle, &config)) {
    fprintf(stderr, "swupdate:: Failed to add suricatta options to config\n");
    config_destroy(&config);
    return false;
  }
  if (!prv_swupdate_add_identify(handle, &config)) {
    fprintf(stderr, "swupdate:: Failed to add identify options to config\n");
    config_destroy(&config);
    return false;
  }
  if (!prv_swupdate_add_runtime()) {
    fprintf(stderr, "swupdate:: Failed to add runtime override options "
                    "to config\n");
    config_destroy(&config);
    return false;
  }

  const char *output_file;
  if (!ticosd_get_string(handle->ticosd, "swupdate_plugin", "output_file", &output_file)) {
    output_file = DEFAULT_OUTPUT_FILE;
  }

  if (!config_write_file(&config, output_file)) {
    fprintf(stderr, "swupdate:: Failed to write config file to '%s'\n", output_file);
    config_destroy(&config);
    return false;
  }

  config_destroy(&config);

  return true;
}

/**
 * @brief Destroys swupdate plugin
 *
 * @param ticosd swupdate plugin handle
 */
static void prv_swupdate_destroy(sTicosdPlugin *handle) {
  if (handle) {
    free(handle);
  }
}

/**
 * @brief Reload swupdate plugin
 *
 * @param handle swupdate plugin handle
 * @return true Successfully reloaded swupdate plugin
 * @return false Failed to reload plugin
 */
static bool prv_swupdate_reload(sTicosdPlugin *handle) {
  if (!prv_swupdate_generate_config(handle)) {
    fprintf(stderr, "swupdate:: Failed to generate config file\n");
    return false;
  }
  if (!ticosd_restart_service_if_running("swupdate.service")) {
    fprintf(stderr, "swupdate:: Failed to restart swupdate\n");
    return false;
  }
  // We need to also reload swupdate.socket otherwise the IPC communication to
  // swupdate is broken.
  if (!ticosd_restart_service_if_running("swupdate.socket")) {
    fprintf(stderr, "swupdate:: Failed to restart swupdate.socket\n");
    return false;
  }

  return true;
}

static sTicosdPluginCallbackFns s_fns = {
  .plugin_destroy = prv_swupdate_destroy,
  .plugin_reload = prv_swupdate_reload,
};

/**
 * @brief Initialises swupdate plugin
 *
 * @param ticosd Main ticosd handle
 * @return callbackFunctions_t Plugin function table
 */
bool ticosd_swupdate_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns) {
  sTicosdPlugin *handle = calloc(sizeof(sTicosdPlugin), 1);
  if (!handle) {
    fprintf(stderr, "swupdate:: Failed to allocate plugin handle\n");
    return false;
  }

  handle->ticosd = ticosd;
  *fns = &s_fns;
  (*fns)->handle = handle;

  // Ignore failures after this point as we still want setting changes to attempt to reload the
  // config

  prv_swupdate_reload(handle);

  return true;
}

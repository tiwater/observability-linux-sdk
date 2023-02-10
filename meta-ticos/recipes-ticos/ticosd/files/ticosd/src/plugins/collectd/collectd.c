//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ticos collectd plugin implementation

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <systemd/sd-bus.h>
#include <unistd.h>

#include "ticos/core/math.h"
#include "ticos/util/string.h"
#include "ticos/util/systemd.h"
#include "ticosd.h"

#define DEFAULT_HEADER_INCLUDE_OUTPUT_FILE "/tmp/collectd-header-include.conf"
#define DEFAULT_FOOTER_INCLUDE_OUTPUT_FILE "/tmp/collectd-footer-include.conf"
#define DEFAULT_INTERVAL_SECS 3600

#define COLLECTD_PATH "/logstash"
#define TICOS_HEADER "Ticos-Project-Key"

struct TicosdPlugin {
  sTicosd *ticosd;
  bool was_enabled;
  const char *header_include_output_file;
  const char *footer_include_output_file;
};

/**
 * @brief Generate new collectd-header-include.conf file from config
 *
 * @param handle collectd plugin handle
 * @param override_interval if greater than 0, will override the interval in configuration
 * @return true Successfully generated new config
 * @return false Failed to generate
 */
bool prv_generate_header_include(sTicosdPlugin *handle, int override_interval) {
  bool result = true;
  FILE *fd = NULL;

  int interval_seconds = DEFAULT_INTERVAL_SECS;
  ticosd_get_integer(handle->ticosd, "collectd_plugin", "interval_seconds",
                        &interval_seconds);

  if (override_interval > 0) {
    interval_seconds = override_interval;
  }

  fd = fopen(handle->header_include_output_file, "w+");
  if (!fd) {
    fprintf(stderr, "collectd:: Failed to open output file: %s\n",
            handle->header_include_output_file);
    result = false;
    goto cleanup;
  }

  char *globals_fmt = "Interval %d\n\n";
  if (fprintf(fd, globals_fmt, interval_seconds) == -1) {
    fprintf(stderr, "collectd:: Failed to write Interval\n");
    result = false;
    goto cleanup;
  }

cleanup:
  if (fd != NULL) {
    fclose(fd);
  }
  return result;
}

bool prv_generate_write_http(sTicosdPlugin *handle, FILE *fd) {
  const sTicosdDeviceSettings *settings = ticosd_get_device_settings(handle->ticosd);

  const char *base_url, *software_type, *software_version, *project_key;
  if (!ticosd_get_string(handle->ticosd, "", "base_url", &base_url)) {
    return false;
  }
  if (!ticosd_get_string(handle->ticosd, "", "software_type", &software_type)) {
    return false;
  }
  if (!ticosd_get_string(handle->ticosd, "", "software_version", &software_version)) {
    return false;
  }
  if (!ticosd_get_string(handle->ticosd, "", "project_key", &project_key)) {
    return false;
  }
  int interval_seconds = 0;
  ticosd_get_integer(handle->ticosd, "collectd_plugin", "interval_seconds",
                        &interval_seconds);

  bool result = true;
  char *url = NULL;
  char *add_header = NULL;

  int buffer_size = 64;
  ticosd_get_integer(handle->ticosd, "collectd_plugin", "write_http_buffer_size_kib",
                        &buffer_size);
  buffer_size *= 1024;

  // Future: read from remote Ticos device config.
  bool store_rates = true;  // Otherwise most metrics are reported as cumulative values.
  int low_speed_limit = 0;
  int timeout = 0;

  char *url_fmt = "%s%s";
  if (ticos_asprintf(&url, url_fmt, base_url, COLLECTD_PATH) == -1) {
    fprintf(stderr, "collectd:: Failed to create url buffer\n");
    result = false;
    goto cleanup;
  }

  char *add_header_fmt = "%s: %s";
  if (ticos_asprintf(&add_header, add_header_fmt, TICOS_HEADER, project_key) == -1) {
    fprintf(stderr, "collectd:: Failed to create additional headers buffer\n");
    result = false;
    goto cleanup;
  }

  char *write_http_fmt = "<LoadPlugin write_http>\n"
                         "  FlushInterval %d\n"
                         "</LoadPlugin>\n\n"
                         "<Plugin write_http>\n"
                         "  <Node \"ticos\">\n"
                         "    URL \"%s\"\n"
                         "#    VerifyPeer true\n"
                         "#    VerifyHost true\n"
                         "    Header \"%s\"\n"
                         "    Header \"deviceId: %s\"\n"
                         "    Header \"hardwareVersion: %s\"\n"
                         "    Header \"softwareType: %s\"\n"
                         "    Header \"softwareVersion: %s\"\n"
                         "    Format \"JSON\"\n"
                         "    Metrics true\n"
                         "    Notifications false\n"
                         "    StoreRates %s\n"
                         "    BufferSize %d\n"
                         "    LowSpeedLimit %d\n"
                         "    Timeout %d\n"
                         "  </Node>\n"
                         "</Plugin>\n\n";
  if (fprintf(fd, write_http_fmt, interval_seconds, url, add_header, settings->device_id, settings->hardware_version, software_type,
        software_version, store_rates ? "true" : "false",
              buffer_size, low_speed_limit, timeout) == -1) {
    fprintf(stderr, "collectd:: Failed to write write_http statement\n");
    result = false;
    goto cleanup;
  }

cleanup:
  free(url);
  free(add_header);

  return result;
}

bool prv_generate_chain(sTicosdPlugin *handle, FILE *fd) {
  // TODO: Add filtering once structure has been agreed on
  bool result = true;
  const char *non_ticos_chain;
  char *target = NULL;

  if (!ticosd_get_string(handle->ticosd, "collectd_plugin", "non_ticosd_chain",
                            &non_ticos_chain) ||
      strlen(non_ticos_chain) == 0) {
    target = strdup("    Target \"stop\"\n");
  } else {
    char *target_fmt = "    <Target \"jump\">\n"
                       "      Chain \"%s\"\n"
                       "    </Target>";
    if (ticos_asprintf(&target, target_fmt, non_ticos_chain) == -1) {
      fprintf(stderr, "collectd:: Failed to create target buffer\n");
      result = false;
      goto cleanup;
    }
  }

  char *chain_fmt = "LoadPlugin match_regex\n"
                    "PostCacheChain \"TicosdGeneratedPostCacheChain\"\n"
                    "<Chain \"TicosdGeneratedPostCacheChain\">\n"
                    "  <Rule \"ignore_memory_metrics\">\n"
                    "    <Match \"regex\">\n"
                    "      Type \"^memory$\"\n"
                    "      TypeInstance \"^(buffered|cached|slab_recl|slab_unrecl)$\"\n"
                    "    </Match>\n"
                    "%s"
                    "  </Rule>\n"
                    "  Target \"write\"\n"
                    "</Chain>\n\n";
  if (fprintf(fd, chain_fmt, target) == -1) {
    fprintf(stderr, "collectd:: Failed to create chain buffer\n");
    result = false;
    goto cleanup;
  }

cleanup:
  free(target);

  return result;
}

/**
 * @brief Generate new collectd-postamble.conf file from config
 *
 * @param handle collectd plugin handle
 * @return true Successfully generated new config
 * @return false Failed to generate
 */
static bool prv_generate_footer_include(sTicosdPlugin *handle) {
  bool result = true;

  FILE *fd = fopen(handle->footer_include_output_file, "w+");
  if (!fd) {
    fprintf(stderr, "collectd:: Failed to open output file: %s\n",
            handle->footer_include_output_file);
    result = false;
    goto cleanup;
  }

  if (!prv_generate_write_http(handle, fd)) {
    result = false;
    goto cleanup;
  }

  if (!prv_generate_chain(handle, fd)) {
    result = false;
    goto cleanup;
  }

cleanup:
  if (fd != NULL) {
    fclose(fd);
  }
  return result;
}

/**
 * @brief Destroys collectd plugin
 *
 * @param ticosd collectd plugin handle
 */
static void prv_destroy(sTicosdPlugin *handle) {
  if (handle) {
    free(handle);
  }
}

/**
 * @brief Gets the size of the file for the given file path.
 * @param file_path The file path.
 * @return Size of the file or -errno in case of an error.
 */
static ssize_t prv_get_file_size(const char *file_path) {
  if (access(file_path, F_OK) != 0) {
    return -errno;
  }
  struct stat st;
  stat(file_path, &st);
  return st.st_size;
}

/**
 * @brief Empties the given file for the given file path.
 * @param file_path The file path.
 */
static bool prv_write_empty_file(const char *file_path) {
  FILE *fd = fopen(file_path, "w+");
  if (!fd) {
    fprintf(stderr, "collectd:: Failed to open output file: %s\n", file_path);
    return false;
  }
  fclose(fd);
  return true;
}

/**
 * Clears the config files, but only if they are not already cleared.
 * @param handle collectd plugin handle
 * @return True if one or more files were cleared, or false if all files had already been cleared,
 * or the files did not exist before.
 */
static bool prv_clear_config_files_if_not_already_cleared(sTicosdPlugin *handle) {
  bool did_clear = false;
  const char *output_files[] = {
    handle->header_include_output_file,
    handle->footer_include_output_file,
  };
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(output_files); ++i) {
    const ssize_t rv = prv_get_file_size(output_files[i]);
    const size_t should_clear = rv > 0;
    if (should_clear || rv == -ENOENT /* file does not exist yet */) {
      prv_write_empty_file(output_files[i]);
    }
    did_clear |= should_clear != 0;
  }
  return did_clear;
}

/**
 * @brief Reload collectd plugin
 *
 * @param handle collectd plugin handle
 * @return true Successfully reloaded collectd plugin
 * @return false Failed to reload plugin
 */
static bool prv_reload(sTicosdPlugin *handle) {
  if (!handle) {
    return false;
  }

  bool enabled;
  if (!ticosd_get_boolean(handle->ticosd, "", "enable_data_collection", &enabled) ||
      !enabled) {
    // Data collection is disabled
    fprintf(stderr, "collectd:: Data collection is off, plugin disabled.\n");

    const bool needs_restart = prv_clear_config_files_if_not_already_cleared(handle);

    if (handle->was_enabled || needs_restart) {
      // Data collection only just disabled

      if (!ticosd_restart_service_if_running("collectd.service")) {
        fprintf(stderr, "collectd:: Failed to restart collectd\n");
        return false;
      }

      handle->was_enabled = false;
    }
  } else {
    // Data collection enabled
    if (!prv_generate_header_include(handle, 0)) {
      fprintf(stderr, "collectd:: Failed to generate updated header config file\n");
      return false;
    }
    if (!prv_generate_footer_include(handle)) {
      fprintf(stderr, "collectd:: Failed to generate updated footer config file\n");
      return false;
    }

    if (!ticosd_restart_service_if_running("collectd.service")) {
      fprintf(stderr, "collectd:: Failed to restart collectd\n");
      return false;
    }

    handle->was_enabled = true;
  }
  return true;
}

static bool prv_request_metrics(sTicosdPlugin *handle) {
  if (handle->was_enabled) {
    // Restarting collectd forces a new measurement of all monitored values.
    if (!ticosd_restart_service_if_running("collectd.service")) {
      fprintf(stderr, "collectd:: Failed to restart collectd\n");
      return false;
    }

    // Make sure we give collectd time to take the measurement
    sleep(1);

    // And now force collectd to flush the measurements in cache.
    fprintf(stderr, "collectd:: Requesting metrics from collectd now.\n");
    ticosd_kill_service("collectd.service", SIGUSR1);
  } else {
    fprintf(stderr, "collected:: Metrics are not enabled.\n");
  }
  return true;
}

static bool prv_ipc_handler(sTicosdPlugin *handle, struct msghdr *msg, size_t received_size) {
  // Any IPC message will cause us to request metrics.
  return prv_request_metrics(handle);
}

static sTicosdPluginCallbackFns s_fns = {.plugin_destroy = prv_destroy,
                                            .plugin_reload = prv_reload,
                                            .plugin_ipc_msg_handler = prv_ipc_handler};

/**
 * @brief Initialises collectd plugin
 *
 * @param ticosd Main ticosd handle
 * @return callbackFunctions_t Plugin function table
 */
bool ticosd_collectd_init(sTicosd *ticosd, sTicosdPluginCallbackFns **fns) {
  sTicosdPlugin *handle = calloc(sizeof(sTicosdPlugin), 1);
  if (!handle) {
    fprintf(stderr, "collectd:: Failed to allocate plugin handle\n");
    return false;
  }

  handle->ticosd = ticosd;
  *fns = &s_fns;
  (*fns)->handle = handle;

  ticosd_get_boolean(handle->ticosd, "", "enable_data_collection", &handle->was_enabled);

  if (!ticosd_get_string(handle->ticosd, "collectd_plugin", "header_include_output_file",
                            &handle->header_include_output_file)) {
    handle->header_include_output_file = DEFAULT_HEADER_INCLUDE_OUTPUT_FILE;
  }

  if (!ticosd_get_string(handle->ticosd, "collectd_plugin", "footer_include_output_file",
                            &handle->footer_include_output_file)) {
    handle->footer_include_output_file = DEFAULT_FOOTER_INCLUDE_OUTPUT_FILE;
  }

  // Ignore failures after this point as we still want setting changes to attempt to reload the
  // config

  prv_reload(handle);

  return true;
}

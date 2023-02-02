//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Core functionality and main loop

#include "ticosd.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "ticos/core/math.h"
#include "ticos/util/config.h"
#include "ticos/util/device_settings.h"
#include "ticos/util/disk.h"
#include "ticos/util/dump_settings.h"
#include "ticos/util/ipc.h"
#include "ticos/util/pid.h"
#include "ticos/util/plugins.h"
#include "ticos/util/runtime_config.h"
#include "ticos/util/string.h"
#include "ticos/util/systemd.h"
#include "ticos/util/version.h"
#include "network.h"
#include "queue.h"

#define RX_BUFFER_SIZE 1024
#define PID_FILE "/var/run/ticosd.pid"

struct Ticosd {
  sTicosdQueue *queue;
  sTicosdNetwork *network;
  sTicosdConfig *config;
  sTicosdDeviceSettings *settings;
  bool terminate;
  bool dev_mode;
  pthread_t ipc_thread_id;
  int ipc_socket_fd;
  char ipc_rx_buffer[RX_BUFFER_SIZE];
};

static sTicosd *s_handle;

#define NETWORK_FAILURE_FIRST_BACKOFF_SECONDS 60
#define NETWORK_FAILURE_BACKOFF_MULTIPLIER 2

/**
 * @brief Displays usage information
 *
 */
static void prv_ticosd_usage(void) {
  printf("Usage: ticosd [OPTION]...\n\n");
  printf("      --config-file <file>       : Configuration file\n");
  printf("      --daemonize                : Daemonize process\n");
  printf("      --enable-data-collection   : Enable data collection, will restart the main "
         "ticosd service\n");
  printf("      --disable-data-collection  : Disable data collection, will restart the main "
         "ticosd service\n");
  printf("      --enable-dev-mode          : Enable developer mode (restarts ticosd)\n");
  printf("      --disable-dev-mode         : Disable developer mode (restarts ticosd)\n");
  printf("  -h, --help                     : Display this help and exit\n");
  printf("  -s, --show-settings            : Show settings\n");
  printf("  -v, --version                  : Show version information\n");
}

/**
 * @brief Signal handler
 *
 * @param sig Signal number
 */
static void prv_ticosd_sig_handler(int sig) {
  if (sig == SIGUSR1) {
    // Used to service the TX queue
    // The signal has already woken up the main thread from its sleep(), can simply exit here
    return;
  }

  fprintf(stderr, "ticosd:: Received signal %u, shutting down.\n", sig);
  s_handle->terminate = true;

  // shutdown() the read-side of the socket to abort any in-progress recv() calls
  shutdown(s_handle->ipc_socket_fd, SHUT_RD);
}

/**
 * @brief Process TX queue and transmit messages
 *
 * @param handle Main ticosd handle
 * @return true Successfully processed the queue, sending all valid entries
 * @return false Failed to process
 */
static bool prv_ticosd_process_tx_queue(sTicosd *handle) {
  bool allowed;
  if (!ticosd_get_boolean(handle, NULL, "enable_data_collection", &allowed) || !allowed) {
    return true;
  }

  uint32_t count = 0;
  uint32_t queue_entry_size_bytes;
  uint8_t *queue_entry;
  while ((queue_entry = ticosd_queue_read_head(handle->queue, &queue_entry_size_bytes))) {
    const sTicosdTxData *txdata = (const sTicosdTxData *)queue_entry;

    const char *payload = (const char *)txdata->payload;

    eTicosdNetworkResult rc = kTicosdNetworkResult_ErrorNoRetry;
    switch (txdata->type) {
      case kTicosdTxDataType_RebootEvent:
        rc = ticosd_network_post(handle->network, "/api/v0/events", kTicosdHttpMethod_POST,
                                    payload, NULL, 0);
        break;
      case kTicosdTxDataType_CoreUpload:
      case kTicosdTxDataType_CoreUploadWithGzip: {
        const bool is_gzipped = txdata->type == kTicosdTxDataType_CoreUploadWithGzip;
        rc = ticosd_network_file_upload(handle->network, "/api/v0/upload/elf_coredump", payload,
                                           is_gzipped);
        break;
      }
      case kTicosdTxDataType_Attributes: {
        char *endpoint;
        sTicosdTxDataAttributes *data_attributes = (sTicosdTxDataAttributes *)txdata;

        time_t timestamp;
        memcpy(&timestamp, &data_attributes->timestamp, sizeof(time_t));
        char iso_timestamp[sizeof("2022-11-30T11:24:00Z")];
        strftime(iso_timestamp, sizeof(iso_timestamp), "%FT%TZ", gmtime(&timestamp));

        if (ticos_asprintf(&endpoint, "/api/v0/attributes?device_serial=%s&captured_date=%s",
                              handle->settings->device_id, iso_timestamp) == -1) {
          fprintf(stderr, "ticosd:: Unable to allocate memory for attribute endpoint.\n");
          rc = kTicosdNetworkResult_ErrorRetryLater;
          break;
        }
        rc = ticosd_network_post(handle->network, endpoint, kTicosdHttpMethod_PATCH,
                                    data_attributes->json, NULL, 0);
        free(endpoint);
        break;
      }
      default:
        fprintf(stderr, "ticosd:: Unrecognised queue type '%d'\n", txdata->type);
        break;
    }

    free(queue_entry);
    if (rc == kTicosdNetworkResult_OK || rc == kTicosdNetworkResult_ErrorNoRetry) {
      ticosd_queue_complete_read(handle->queue);
    } else {
      fprintf(stderr, "ticosd:: Network error while processing queue. Will retry...\n");
      // Retry-able error
      return false;
    }
    count++;
  }

  if (ticosd_is_dev_mode(handle)) {
    fprintf(stderr, "ticosd:: Transmitted %i messages to ticos.\n", count);
  }
  return true;
}

/**
 * @brief Daemonize process
 *
 * @return true Successfully daemonized
 * @return false Failed
 */
static bool prv_ticosd_daemonize_process(void) {
  char pid[11] = "";
  if (getuid() != 0) {
    fprintf(stderr, "ticosd:: Cannot daemonize as non-root user, aborting.\n");
  }

  // Use noclose=1 so logs written to stdout/stderr can be viewed using journalctl
  if (daemon(0, 1) == -1) {
    fprintf(stderr, "ticosd:: Failed to daemonize, aborting.\n");
    return false;
  }

  const int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    if (errno == EEXIST) {
      fprintf(stderr, "ticosd:: Daemon already running, aborting.\n");
      return false;
    }
    fprintf(stderr, "ticosd:: Failed to open PID file, aborting.\n");
    return false;
  }

  snprintf(pid, sizeof(pid), "%d\n", getpid());

  if (write(fd, pid, sizeof(pid)) == -1) {
    fprintf(stderr, "ticosd:: Failed to write PID file, aborting.\n");
    close(fd);
    unlink(PID_FILE);
  }

  close(fd);

  return true;
}

/**
 * @brief Main process loop
 *
 * @param handle Main ticosd handle
 */
static void prv_ticosd_process_loop(sTicosd *handle) {
  time_t next_telemetry_poll = 0;
  int override_interval = NETWORK_FAILURE_FIRST_BACKOFF_SECONDS;
  while (!handle->terminate) {
    struct timeval last_wakeup;
    gettimeofday(&last_wakeup, NULL);

    int interval = 1 * 60 * 60;
    ticosd_get_integer(handle, NULL, "refresh_interval_seconds", &interval);

    if (next_telemetry_poll + interval >= last_wakeup.tv_sec) {
      next_telemetry_poll = last_wakeup.tv_sec + interval;
      // Unimplemented: Perform data collection calls
    }

    if (prv_ticosd_process_tx_queue(handle)) {
      // Reset override in preparation of next failure
      override_interval = NETWORK_FAILURE_FIRST_BACKOFF_SECONDS;
    } else {
      // call failed, back off up to the entire update interval
      interval = MIN(override_interval, interval);
      override_interval *= NETWORK_FAILURE_BACKOFF_MULTIPLIER;
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    if (!handle->terminate && last_wakeup.tv_sec + interval > now.tv_sec) {
      sleep(last_wakeup.tv_sec + interval - now.tv_sec);
    }
  }
}

static void ticosd_create_data_dir(sTicosd *handle) {
  const char *data_dir;
  if (!ticosd_get_string(handle, "", "data_dir", &data_dir) || strlen(data_dir) == 0) {
    return;
  }

  struct stat sb;
  if (stat(data_dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
    return;
  }

  if (mkdir(data_dir, 0755) == -1) {
    fprintf(stderr, "ticos:: Failed to create ticos base_dir '%s'\n", data_dir);
    return;
  }
}

static void *prv_ipc_process_thread(void *arg) {
  sTicosd *handle = arg;

  if ((handle->ipc_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    fprintf(stderr, "ticos:: Failed to create listening socket : %s\n", strerror(errno));
    goto cleanup;
  }

  struct sockaddr_un addr = {.sun_family = AF_UNIX};
  strncpy(addr.sun_path, TICOSD_IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);
  if (unlink(TICOSD_IPC_SOCKET_PATH) == -1 && errno != ENOENT) {
    fprintf(stderr, "ticos:: Failed to remove IPC socket file '%s' : %s\n",
            TICOSD_IPC_SOCKET_PATH, strerror(errno));
    goto cleanup;
  }

  if (bind(handle->ipc_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    fprintf(stderr, "ticos:: Failed to bind to listener address() : %s\n", strerror(errno));
    goto cleanup;
  }

  while (!handle->terminate) {
    char ctrl_buf[CMSG_SPACE(sizeof(int))] = {'\0'};

    struct iovec iov[1] = {{.iov_base = handle->ipc_rx_buffer, .iov_len = RX_BUFFER_SIZE}};

    struct sockaddr_un src_addr = {0};

    struct msghdr msg = {
      .msg_name = (struct sockaddr *)&src_addr,
      .msg_namelen = sizeof(src_addr),
      .msg_iov = iov,
      .msg_iovlen = 1,
      .msg_control = ctrl_buf,
      .msg_controllen = sizeof(ctrl_buf),
    };

    size_t received_size;
    if ((received_size = recvmsg(handle->ipc_socket_fd, &msg, 0)) <= 0 || msg.msg_iovlen != 1) {
      continue;
    }

    if (!ticosd_plugins_process_ipc(&msg, received_size)) {
      fprintf(stderr, "ticosd:: Failed to process IPC message (no plugin).\n");
    }
  }

cleanup:
  close(handle->ipc_socket_fd);
  if (unlink(TICOSD_IPC_SOCKET_PATH) == -1 && errno != ENOENT) {
    fprintf(stderr, "ticos:: Failed to remove IPC socket file '%s' : %s\n",
            TICOSD_IPC_SOCKET_PATH, strerror(errno));
  }

  return (void *)NULL;
}

/**
 * @brief Entry function
 *
 * @param argc Argument count
 * @param argv Argument array
 * @return int Return code
 */
int ticosd_main(int argc, char *argv[]) {
  bool daemonize = false;
  bool enable_comms = false;
  bool disable_comms = false;
  bool display_config = false;
  bool enable_devmode = false;
  bool disable_devmode = false;

  s_handle = calloc(sizeof(sTicosd), 1);
  const char *config_file = CONFIG_FILE;

  static struct option sTicosdLongOptions[] = {
    {"config-file", required_argument, NULL, 'c'},
    {"disable-data-collection", no_argument, NULL, 'd'},
    {"disable-dev-mode", no_argument, NULL, 'm'},
    {"enable-data-collection", no_argument, NULL, 'e'},
    {"enable-dev-mode", no_argument, NULL, 'M'},
    {"help", no_argument, NULL, 'h'},
    {"show-settings", no_argument, NULL, 's'},
    {"version", no_argument, NULL, 'v'},
    {"daemonize", no_argument, NULL, 'Z'},
    {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "c:dehsvZ", sTicosdLongOptions, NULL)) != -1) {
    switch (opt) {
      case 'c':
        config_file = optarg;
        break;
      case 'd':
        disable_comms = true;
        break;
      case 'e':
        enable_comms = true;
        break;
      case 'h':
        prv_ticosd_usage();
        exit(EXIT_SUCCESS);
      case 's':
        display_config = true;
        break;
      case 'v':
        ticos_version_print_info();
        exit(EXIT_SUCCESS);
      case 'Z':
        daemonize = true;
        break;
      case 'M':
        enable_devmode = true;
        break;
      case 'm':
        disable_devmode = true;
        break;
      default:
        exit(EXIT_FAILURE);
    }
  }

  if (!(s_handle->config = ticosd_config_init(s_handle, config_file))) {
    fprintf(stderr, "ticosd:: Failed to create config object, aborting.\n");
    exit(EXIT_FAILURE);
  }

  ticosd_create_data_dir(s_handle);

  if (enable_comms || disable_comms) {
    if (enable_comms && disable_comms) {
      fprintf(stderr, "ticosd:: Unable to enable and disable comms simultaneously\n");
      exit(EXIT_FAILURE);
    }
    exit(ticos_set_runtime_bool_and_reload(s_handle->config, CONFIG_KEY_DATA_COLLECTION,
                                              "data collection", enable_comms));
  }

  if (enable_devmode || disable_devmode) {
    if (enable_devmode && disable_devmode) {
      fprintf(stderr, "ticosd:: Unable to enable and disable dev-mode simultaneously\n");
      exit(EXIT_FAILURE);
    }
    exit(ticos_set_runtime_bool_and_reload(s_handle->config, CONFIG_KEY_DEV_MODE,
                                              "developer mode", enable_devmode));
  }

  if (!(s_handle->settings = ticosd_device_settings_init())) {
    fprintf(stderr, "ticosd:: Failed to load all required device settings, aborting.\n");
    exit(EXIT_FAILURE);
  }

  ticosd_dump_settings(s_handle->settings, s_handle->config, config_file);
  if (display_config) {
    /* Already reported above, just exit */
    exit(EXIT_SUCCESS);
  }

  if (!daemonize && ticosd_check_for_pid_file()) {
    fprintf(stderr, "ticosd:: ticosd already running, pidfile: '%s'.\n", PID_FILE);
    exit(EXIT_FAILURE);
  }

  //! Disable coredumping of this process
  prctl(PR_SET_DUMPABLE, 0, 0, 0);

  signal(SIGTERM, prv_ticosd_sig_handler);
  signal(SIGHUP, prv_ticosd_sig_handler);
  signal(SIGINT, prv_ticosd_sig_handler);
  signal(SIGUSR1, prv_ticosd_sig_handler);

  int queue_size = 0;
  ticosd_get_integer(s_handle, NULL, "queue_size_kib", &queue_size);

  if (!(s_handle->queue = ticosd_queue_init(s_handle, queue_size * 1024))) {
    fprintf(stderr, "ticosd:: Failed to create queue object, aborting.\n");
    exit(EXIT_FAILURE);
  }

  bool allowed;
  if (!ticosd_get_boolean(s_handle, NULL, "enable_data_collection", &allowed) || !allowed) {
    ticosd_queue_reset(s_handle->queue);
  }

  if (!(s_handle->network = ticosd_network_init(s_handle))) {
    fprintf(stderr, "ticosd:: Failed to create networking object, aborting.\n");
    exit(EXIT_FAILURE);
  }

  if (ticosd_get_boolean(s_handle, NULL, CONFIG_KEY_DEV_MODE, &s_handle->dev_mode) &&
      s_handle->dev_mode == true) {
    fprintf(stderr, "ticosd:: Starting with developer mode enabled\n");
  }

  ticosd_load_plugins(s_handle);

  if (daemonize && !prv_ticosd_daemonize_process()) {
    exit(EXIT_FAILURE);
  }

  // Ensure startup scroll is pushed to journal
  fflush(stdout);

  if (pthread_create(&s_handle->ipc_thread_id, NULL, prv_ipc_process_thread, s_handle) != 0) {
    fprintf(stderr, "ipc:: Failed to create handler thread\n");
    exit(EXIT_FAILURE);
  }

  prv_ticosd_process_loop(s_handle);

  pthread_join(s_handle->ipc_thread_id, NULL);

  ticosd_destroy_plugins();

  ticosd_network_destroy(s_handle->network);
  ticosd_queue_destroy(s_handle->queue);
  ticosd_config_destroy(s_handle->config);
  ticosd_device_settings_destroy(s_handle->settings);
  free(s_handle);

  if (daemonize) {
    unlink(PID_FILE);
  }
  return 0;
}

/**
 * @brief Plugin API impl for queueing data to transmit
 *
 * @param handle Main ticosd handle
 * @param data Data to transmit
 * @param payload_size Size of the data->payload buffer
 * @return true Successfully queued for transmitting
 * @return false Failed to queue
 */
bool ticosd_txdata(sTicosd *handle, const sTicosdTxData *data, uint32_t payload_size) {
  bool allowed;
  if (!ticosd_get_boolean(handle, "", "enable_data_collection", &allowed) || !allowed) {
    return true;
  }
  return ticosd_queue_write(handle->queue, (const uint8_t *)data,
                               sizeof(sTicosdTxData) + payload_size);
}

bool ticosd_get_boolean(sTicosd *handle, const char *parent_key, const char *key, bool *val) {
  return ticosd_config_get_boolean(handle->config, parent_key, key, val);
}
bool ticosd_get_integer(sTicosd *handle, const char *parent_key, const char *key, int *val) {
  return ticosd_config_get_integer(handle->config, parent_key, key, val);
}
bool ticosd_get_string(sTicosd *handle, const char *parent_key, const char *key,
                          const char **val) {
  return ticosd_config_get_string(handle->config, parent_key, key, val);
}

bool ticosd_get_objects(sTicosd *handle, const char *parent_key,
                           sTicosdConfigObject **objects, int *len) {
  return ticosd_config_get_objects(handle->config, parent_key, objects, len);
}

const sTicosdDeviceSettings *ticosd_get_device_settings(sTicosd *ticosd) {
  return ticosd->settings;
}

char *ticosd_generate_rw_filename(sTicosd *handle, const char *filename) {
  return ticosd_config_generate_rw_filename(handle->config, filename);
}

bool ticosd_is_dev_mode(sTicosd *handle) { return handle->dev_mode; }

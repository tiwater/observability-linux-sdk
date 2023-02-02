//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!

#include "ticos/util/ipc.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "ticos/util/pid.h"
#include "ticosd.h"

bool ticosd_ipc_sendmsg(uint8_t *msg, size_t len) {
  int fd;

  // Create socket
  if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    fprintf(stderr, "Failed to create socket() : %s\n", strerror(errno));
    return false;
  }

  // Setup for Non-connected UNIX socket to ticosd
  struct sockaddr_un server_addr = {.sun_family = AF_UNIX};
  strncpy(server_addr.sun_path, TICOSD_IPC_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

  // Send message to ticosd
  ssize_t result =
    sendto(fd, msg, len, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
  if (result == -1) {
    fprintf(stderr, "Failed to communicate with ticosd : %s\n", strerror(errno));
  } else if (result > 0 && (size_t)result < len) {
    fprintf(stderr, "Message was only partially sent. Should not happen.");
  }

  close(fd);
  return (size_t)result == len;
}

bool ticosd_send_flush_queue_signal(void) {
  int pid = ticosd_get_pid();
  if (pid == -1) {
    fprintf(stderr, "Unable to read ticosd PID file.\n");
    return false;
  }
  if (kill(pid, SIGUSR1) < 0) {
    fprintf(stderr, "Unable to send USR1 signal to ticosd: %s\n", strerror(errno));
    return false;
  }
  return true;
}

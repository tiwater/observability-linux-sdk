#
# Copyright (c) Ticos, Inc.
# See License.txt for details
import time

import pexpect
from qemu import QEMU


def test_start(qemu: QEMU):
    qemu.exec_cmd("ticosd --enable-dev-mode")
    qemu.child().expect("Enabling developer mode")
    qemu.systemd_wait_for_service_state("ticosd.service", "active")

    qemu.exec_cmd("ticosd --enable-dev-mode")
    qemu.child().expect("Developer mode is already enabled")

    qemu.exec_cmd("ticosd --disable-dev-mode")
    qemu.child().expect("Disabling developer mode")

    # Work-around for checking the state in the next line before ticosd has even restarted:
    # FIXME: track the journal logs instead of using `systemctl is-active`
    time.sleep(0.5)

    qemu.systemd_wait_for_service_state("ticosd.service", "active")

    qemu.exec_cmd("ticosd --disable-dev-mode")
    qemu.child().expect("Developer mode is already disabled")

    # Check that the service restarted:
    qemu.exec_cmd("journalctl -u ticosd.service")
    qemu.child().expect("Stopped ticosd daemon")
    qemu.child().expect("Starting ticosd daemon")
    qemu.child().expect("Starting with developer mode enabled")

    # Check that the service did not fail:
    # FIXME: pexpect does not have a "not expecting" API :/
    qemu.exec_cmd("journalctl -u ticosd.service")
    output = b""
    while True:
        try:
            output += qemu.child().read_nonblocking(size=1024, timeout=0.5)
        except pexpect.TIMEOUT:
            break
    assert b"ticosd.service: Scheduled restart job" not in output

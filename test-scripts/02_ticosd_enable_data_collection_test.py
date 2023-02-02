#
# Copyright (c) Ticos, Inc.
# See License.txt for details
import time

import pexpect
from qemu import QEMU


def test_start(qemu: QEMU):
    qemu.exec_cmd("ticosd --enable-data-collection")
    qemu.child().expect("Enabling data collection")
    qemu.systemd_wait_for_service_state("ticosd.service", "active")

    qemu.exec_cmd("ticosd --enable-data-collection")
    qemu.child().expect("Data collection is already enabled.")

    qemu.exec_cmd("ticosd --disable-data-collection")
    qemu.child().expect("Disabling data collection.")

    # Work-around for checking the state in the next line before ticosd has even restarted:
    # FIXME: track the journal logs instead of using `systemctl is-active`
    time.sleep(0.5)

    qemu.systemd_wait_for_service_state("ticosd.service", "active")

    qemu.exec_cmd("ticosd --disable-data-collection")
    qemu.child().expect("Data collection is already disabled.")

    # Check that the service restarted:
    qemu.exec_cmd("journalctl -u ticosd.service")
    qemu.child().expect("Stopped ticosd daemon")
    qemu.child().expect("Starting ticosd daemon")

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


def test_via_ticosctl(qemu: QEMU):
    qemu.exec_cmd("ticosctl enable-data-collection")
    qemu.child().expect("Enabling data collection.")
    qemu.systemd_wait_for_service_state("ticosd.service", "active")

    qemu.exec_cmd("ticosctl enable-data-collection")
    qemu.child().expect("Data collection is already enabled.")

    qemu.exec_cmd("ticosctl disable-data-collection")
    qemu.child().expect("Disabling data collection.")

    # Work-around for checking the state in the next line before ticosd has even restarted:
    # FIXME: track the journal logs instead of using `systemctl is-active`
    time.sleep(0.5)

    qemu.systemd_wait_for_service_state("ticosd.service", "active")

    qemu.exec_cmd("ticosctl disable-data-collection")
    qemu.child().expect("Data collection is already disabled.")

    # Check that the service restarted:
    qemu.exec_cmd("journalctl -u ticosd.service")
    qemu.child().expect("Stopped ticosd daemon")
    qemu.child().expect("Starting ticosd daemon")

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

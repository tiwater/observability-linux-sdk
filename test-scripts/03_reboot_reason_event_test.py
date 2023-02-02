#
# Copyright (c) Ticos, Inc.
# See License.txt for details
from ticos_service_tester import TicosServiceTester
from qemu import QEMU

# Assumptions:
# - The machine/qemu is built with a valid project key of a project on app.ticos.com,
#   or whatever the underlying QEMU instance points at.
# - The TICOS_E2E_* environment variables are set to match whatever the underlying
#   QEMU instance points at.


def test_reboot_reason_user_reset(
    qemu: QEMU, ticos_service_tester: TicosServiceTester, qemu_device_id: str
):
    # enable data collection, so that the reboot event can get captured
    qemu.exec_cmd("ticosd --enable-data-collection")

    # Stream ticosd's log
    qemu.exec_cmd("journalctl --follow --unit=ticosd.service &")

    # Wait until it has been restarted after enabling data collection:
    qemu.child().expect("Stopped ticosd daemon")
    qemu.child().expect("Started ticosd daemon")

    qemu.exec_cmd("reboot")
    qemu.child().expect("reboot: Restarting system")
    qemu.child().expect(" login:")

    events = ticos_service_tester.poll_reboot_events_until_count(
        1, device_serial=qemu_device_id, timeout_secs=60
    )
    assert events
    assert events[0]["reason"] == 2  # User Reset


def test_reboot_reason_already_tracked(
    qemu: QEMU, ticos_service_tester: TicosServiceTester, qemu_device_id: str
):
    # enable data collection, so that the reboot event can get captured
    qemu.exec_cmd("ticosd --enable-data-collection")

    # Stream ticosd's log
    qemu.exec_cmd("journalctl --follow --unit=ticosd.service &")

    # Wait until it has been restarted after enabling data collection:
    qemu.child().expect("Stopped ticosd daemon")
    qemu.child().expect("Started ticosd daemon")

    qemu.exec_cmd("systemctl restart ticosd")

    qemu.child().expect("reboot:: boot_id already tracked")


def test_reboot_reason_api(
    qemu: QEMU, ticos_service_tester: TicosServiceTester, qemu_device_id: str
):
    # enable data collection, so that the reboot event can get captured
    qemu.exec_cmd("ticosctl enable-data-collection")

    # Stream ticosd's log
    qemu.exec_cmd("journalctl --follow --unit=ticosd.service &")

    # Wait until it has been restarted after enabling data collection:
    qemu.child().expect("Stopped ticosd daemon")
    qemu.child().expect("Started ticosd daemon")

    # Reboot with code 4 "Low Power"
    qemu.exec_cmd("ticosctl reboot --reason 4")
    qemu.child().expect("reboot: Restarting system")
    qemu.child().expect(" login:")

    events = ticos_service_tester.poll_reboot_events_until_count(
        1, device_serial=qemu_device_id, timeout_secs=60
    )
    assert events
    assert events[0]["reason"] == 4

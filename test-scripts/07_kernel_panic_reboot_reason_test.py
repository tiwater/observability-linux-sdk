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
def test_start(
    qemu: QEMU, ticos_service_tester: TicosServiceTester, qemu_device_id: str
):
    # enable data collection, so that the reboot event can get captured
    qemu.exec_cmd("ticosd --enable-data-collection")

    # Stream ticosd's log
    qemu.exec_cmd("journalctl --follow --unit=ticosd.service &")

    # Wait until it has been restarted after enabling data collection:
    qemu.child().expect("Stopped ticosd daemon")
    qemu.child().expect("Started ticosd daemon")

    # Sync filesystems (otherwise /media/ticos/runtime.conf would sometimes be lost!)
    qemu.exec_cmd("sync")

    qemu.exec_cmd("echo 1 > /proc/sys/kernel/panic")
    qemu.exec_cmd("echo c > /proc/sysrq-trigger")
    qemu.child().expect(" login:")

    events = ticos_service_tester.poll_reboot_events_until_count(
        1, device_serial=qemu_device_id, timeout_secs=60
    )
    assert events
    assert events[0]["reason"] == 0x8008

    qemu.child().sendline("root")
    qemu.exec_cmd("reboot")
    qemu.child().expect("reboot: Restarting system")
    qemu.child().expect(" login:")

    events = ticos_service_tester.poll_reboot_events_until_count(
        2, device_serial=qemu_device_id, timeout_secs=60
    )
    assert events
    assert events[0]["reason"] == 2

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
def test(
    qemu: QEMU, ticos_service_tester: TicosServiceTester, qemu_device_id: str
):
    qemu.exec_cmd(
        'echo \'{"collectd_plugin": {"interval_seconds": 5}}\' > /media/ticos/runtime.conf'
    )
    qemu.exec_cmd("ticosd --enable-data-collection")

    qemu.systemd_wait_for_service_state("ticosd.service", "active")
    qemu.systemd_wait_for_service_state("collectd.service", "active")

    def _check():
        reports = ticos_service_tester.list_reports(
            dict(device_serial=qemu_device_id),
            ignore_errors=True,
        )
        assert reports
        # Note: sometimes the first heartbeat is an empty dict:
        assert any((report["metrics"] for report in reports))

    ticos_service_tester.poll_until_not_raising(
        _check, timeout_seconds=60, poll_interval_seconds=1
    )

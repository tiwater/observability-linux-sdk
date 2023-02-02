#
# Copyright (c) Ticos, Inc.
# See License.txt for details
import time

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
    qemu.exec_cmd("ticosctl enable-data-collection")

    # Start following ticosd logs
    qemu.exec_cmd("journalctl -u ticosd -n 0 -f &")

    qemu.systemd_wait_for_service_state("ticosd.service", "active")
    qemu.systemd_wait_for_service_state("collectd.service", "active")

    # Wait until ticosd has started
    qemu.child().expect("Started ticosd daemon.")

    # Wait a little bit for any "startup requests"
    time.sleep(3)

    qemu.exec_cmd("ticosctl request-metrics")

    # Wait for message indicating that collectd has been poked
    qemu.child().expect("collectd:: Requesting metrics from collectd now.")

    # ticosd waits one second + we want to make sure data has been flushed to ticos and processed
    time.sleep(5)

    # Wait until we have received at least one valid report.
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

    # Now check how many reports we got
    reports = ticos_service_tester.list_reports(
        dict(device_serial=qemu_device_id),
        ignore_errors=True,
    )
    # And make sure we have at least one
    assert [list(report for report in reports if report["metrics"])]

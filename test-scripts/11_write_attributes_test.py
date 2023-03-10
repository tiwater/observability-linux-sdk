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
    qemu.exec_cmd("ticosctl enable-data-collection")

    # Start following ticosd logs
    qemu.exec_cmd("journalctl -u ticosd -n 0 -f &")

    # Wait until ticosd has started
    qemu.child().expect("Started ticosd daemon.")

    # Write attributes
    qemu.exec_cmd(
        'ticosctl write-attributes a_string=running a_bool=false a_boolish_string=\\"true\\" a_float=42.42'
    )

    # Poke ticosd to upload now
    qemu.exec_cmd("ticosctl sync")

    # Wait until we have received attributes
    def _check():
        attributes = ticos_service_tester.list_attributes(
            device_serial=qemu_device_id
        )

        assert attributes

        d = {
            a["custom_metric"]["string_key"]: a["state"]["value"]
            for a in attributes
            if a["state"] is not None
        }

        assert d["a_string"] == "running"
        assert d["a_bool"] is False
        assert d["a_boolish_string"] == "true"
        assert d["a_float"] == 42.42

    ticos_service_tester.poll_until_not_raising(
        _check, timeout_seconds=60, poll_interval_seconds=1
    )

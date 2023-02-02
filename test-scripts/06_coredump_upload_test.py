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
#
# If you want to see the crashes on the server, remember to upload the symbols:
# qemu$ ticos --org $TICOS_E2E_ORGANIZATION_SLUG --org-token $TICOS_E2E_ORG_TOKEN \
#   --project $TICOS_E2E_PROJECT_SLUG upload-yocto-symbols
#   --image tmp/deploy/images/qemuarm64/base-image-qemuarm64.tar.bz2
def test(
    qemu: QEMU, ticos_service_tester: TicosServiceTester, qemu_device_id: str
):
    # Enable data collection, activating the coredump functionality
    qemu.exec_cmd("ticosd --enable-data-collection")
    qemu.systemd_wait_for_service_state("ticosd.service", "active")

    # Stream ticosd's log
    qemu.exec_cmd("journalctl -n 0 --follow --unit=ticosd.service &")

    # Wait for ticosd to actually be ready
    qemu.child().expect("Started ticosd daemon")

    # Trigger the coredump
    qemu.exec_cmd("ticosctl trigger-coredump")

    # Ensure ticosd has received the core
    qemu.child().expect("coredump:: enqueued corefile")

    # Tell ticos to do the upload now
    qemu.exec_cmd("systemctl kill ticosd --signal SIGUSR1")

    # Ensure ticosd has transmitted the corefile
    qemu.child().expect("network:: Successfully transmitted file")

    # Check that the backend created the coredump:
    ticos_service_tester.poll_elf_coredumps_until_count(
        1, device_serial=qemu_device_id, timeout_secs=60
    )

    # TODO: upload symbol files, so we can assert that the processing was w/o errors here and an issue got created.

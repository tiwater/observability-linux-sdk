#
# Copyright (c) Ticos, Inc.
# See License.txt for details
import os
import pathlib
import shutil
import subprocess
import textwrap
import uuid
from typing import Iterable

import pytest
import runqemu

# Ensure pytest rewrites asserts for better debuggability.
# https://docs.pytest.org/en/stable/writing_plugins.html#assertion-rewriting
pytest.register_assert_rewrite(
    "ticos_service_tester",
)

from ticos_service_tester import TicosServiceTester  # noqa: E402 M900
from qemu import QEMU  # noqa: E402 M900


@pytest.fixture()
def qemu_device_id() -> str:
    device_id = str(uuid.uuid4())
    # Let's leave this here to make debugging failing tests in CI a bit easier:
    print(f"TICOS_DEVICE_ID={device_id}")
    return device_id


@pytest.fixture()
def qemu_hardware_version() -> str:
    return os.environ.get("TICOS_HARDWARE_VERSION", "qemuarm64")


@pytest.fixture()
def ticos_device_info(tmpdir, qemu_device_id, qemu_hardware_version) -> pathlib.Path:
    fn = tmpdir / "ticos-device-info"
    with open(fn, "w") as f:
        f.write(
            textwrap.dedent(
                f"""\
                #!/bin/sh
                echo TICOS_DEVICE_ID={qemu_device_id}
                echo TICOS_HARDWARE_VERSION={qemu_hardware_version}
                """
            )
        )
    os.chmod(fn, 0o755)
    return fn


CI_IMAGE_FILENAME = "ci-test-image.wic"


@pytest.fixture()
def qemu_image_wic_path(tmpdir, ticos_device_info) -> pathlib.Path:
    src_wic = runqemu.qemu_get_image_wic_path(CI_IMAGE_FILENAME)
    dest_wic = tmpdir / "ci-test-image.copy.wic"

    shutil.copyfile(src_wic, dest_wic)

    partition_num = 2

    # Remove /usr/bin/ticos-device-info:
    subprocess.check_output(
        ["wic", "rm", f"{dest_wic}:{partition_num}/usr/bin/ticos-device-info"]
    )
    # Copy in the new /usr/bin/ticos-device-info:
    subprocess.check_output(
        [
            "wic",
            "cp",
            ticos_device_info,
            f"{dest_wic}:{partition_num}/usr/bin/",
        ]
    )

    return dest_wic


@pytest.fixture()
def qemu(qemu_image_wic_path) -> QEMU:
    return QEMU(qemu_image_wic_path)


@pytest.fixture()
def swupdate_enabled() -> bool:
    return False


@pytest.fixture(autouse=True)
def disable_swupdate_if_needed(swupdate_enabled, qemu):
    if swupdate_enabled:
        return

    qemu.exec_cmd("systemctl stop swupdate")
    qemu.exec_cmd("systemctl disable swupdate")


@pytest.fixture()
def ticos_service_tester() -> Iterable[TicosServiceTester]:
    st = TicosServiceTester(
        base_url=os.environ["TICOS_E2E_API_BASE_URL"],
        organization_slug=os.environ["TICOS_E2E_ORGANIZATION_SLUG"],
        project_slug=os.environ["TICOS_E2E_PROJECT_SLUG"],
        organization_token=os.environ["TICOS_E2E_ORG_TOKEN"],
    )
    yield st

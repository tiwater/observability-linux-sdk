#
# Copyright (c) Ticos, Inc.
# See License.txt for details
from qemu import QEMU


def test_start(qemu: QEMU):
    qemu.child().sendline("ticosd -h")
    qemu.child().expect("Usage: ticosd \\[OPTION\\]...")

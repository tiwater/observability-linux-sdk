#
# Copyright (c) Ticos, Inc.
# See License.txt for details
import dataclasses
import time
from typing import Any, Callable, Optional, TypeVar
from unittest.mock import ANY

import requests

_T = TypeVar("_T")


@dataclasses.dataclass
class TicosServiceTester:
    base_url: str
    organization_slug: str
    project_slug: str
    organization_token: str

    session: requests.Session = dataclasses.field(init=False)

    def __post_init__(self):
        self.base_url = self.base_url.rstrip("/")
        self.session = requests.Session()
        self.session.auth = ("", self.organization_token)

    @property
    def _api_url(self):
        return f"{self.base_url}/api/v0"

    @property
    def _project_url(self):
        return f"{self._api_url}/organizations/{self.organization_slug}/projects/{self.project_slug}"

    def poll_until_not_raising(
        self,
        check_callback: Callable[[], _T],
        timeout_seconds: float = 10,
        poll_interval_seconds: float = 0.5,
    ) -> _T:
        timeout = time.time() + timeout_seconds
        while True:
            try:
                rv = check_callback()
                return rv
            except Exception:  # noqa
                if time.time() > timeout:
                    raise
            time.sleep(poll_interval_seconds)

    def list_reboot_events(self, *, device_serial, params=None, expect_status=200):
        if params is None:
            params = {}

        url = f"{self._project_url}/devices/{device_serial}/reboots"
        resp = self.session.get(url, params=params)
        assert resp.status_code == expect_status
        return resp.json()["data"] if resp.ok else None

    def poll_reboot_events_until_count(
        self,
        count: int,
        device_serial: str,
        params: Optional[dict] = None,
        timeout_secs: int = 10,
    ) -> Any:
        def _check():
            events = self.list_reboot_events(
                device_serial=device_serial, params=params, expect_status=ANY
            )
            assert len(events) >= count
            return events

        return self.poll_until_not_raising(_check, timeout_seconds=timeout_secs)

    def list_reports(
        self,
        params: Optional[dict] = None,
        expect_status: int = 200,
        ignore_errors: bool = False,
    ):
        rv = self.session.get(f"{self._project_url}/reports", params=params)
        assert rv.status_code == expect_status or ignore_errors
        return rv.json()["data"] if rv.status_code == 200 else []

    def poll_reports_until_count(
        self,
        count: int,
        device_serial: Optional[str] = None,
        params: Optional[dict] = None,
        timeout_secs: int = 10,
    ) -> Any:
        if not params:
            params = {}
        if device_serial:
            params = {"device_serial": device_serial, **params}

        num_reports = 0
        timeout = time.time() + timeout_secs
        reports = None
        while True:
            if num_reports >= count or time.time() > timeout:
                assert num_reports >= count, f"Got: {num_reports}, Expected: {count}"
                break
            reports = self.list_reports(
                params,
                ignore_errors=True,
            )
            num_reports = len(reports)
            time.sleep(0.5)
        return reports

    def list_elf_coredumps(
        self,
        params: Optional[dict] = None,
        expect_status: int = 200,
        ignore_errors: bool = False,
    ):
        rv = self.session.get(f"{self._project_url}/elf_coredumps", params=params)
        assert rv.status_code == expect_status or ignore_errors
        return rv.json()["data"] if rv.status_code == 200 else []

    def poll_elf_coredumps_until_count(
        self,
        count: int,
        device_serial: Optional[str] = None,
        params: Optional[dict] = None,
        timeout_secs: int = 10,
    ) -> Any:
        if not params:
            params = {}
        if device_serial:
            params = {"device": device_serial, **params}

        def _check():
            elf_coredumps = self.list_elf_coredumps(params=params, expect_status=ANY)
            assert len(elf_coredumps) >= count
            return elf_coredumps

        return self.poll_until_not_raising(_check, timeout_seconds=timeout_secs)

    def list_attributes(self, *, device_serial, params=None, expect_status=200):
        if params is None:
            params = {}

        url = f"{self._project_url}/devices/{device_serial}/attributes"
        resp = self.session.get(url, params=params)
        assert resp.status_code == expect_status
        return resp.json()["data"] if resp.ok else None

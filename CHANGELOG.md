# Ticos Linux SDK Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - 2022-12-26

### Added

- [ticosctl] Added a new command `ticosctl` to interact with `ticosd`.
  - `ticosctl trigger-coredump` to force a coredump generation and upload.
  - `ticosctl request-metrics` to force `collectd` to flush metrics to
    Ticos.
  - `ticosctl reboot` to save a reboot reason and restart the system.
  - `ticosctl sync` to process `ticosd` queue immediately.
  - `ticosctl write-attributes` to push device attributes to Ticos.
  - 'Developer Mode` to reduce rate limits applied to coredumps during
    development.

### Changed

- Our Docker container now runs on Apple silicon without Rosetta emulation.
- Updated the `ticos-cli` package in the Docker image.
- Added "preferred versions" for `swupdate` and `collectd`.
- Coredumps are now compressed with gzip reducing storage and network usage.
- `ticosd` is now built with `-g3`.

### Deprecated

- `ticosd --(enable|disable)-dev-collection` and `ticosctl -s` are now
  replaced by equivalent commands on `ticosctl` and will be removed in a
  future version.

### Fixed

- `swupdate` would get in a bad state after reloading `ticosd`. This is fixed
  by restarting both `swupdate` and `swupdate.socket` units.

## [1.1.0] - 2022-11-10

### Added

- [ticosd] A new `last_reboot_reason_file` API has been added to enable
  extending the reboot reason determination subsystem. More information can be
  found in [the documentation of this feature][docs-reboots].
- [ticosd] `ticosd` will now take care of cleaning up `/sys/fs/pstore`
  after a reboot of the system (but only if the reboot reason tracking plugin,
  `plugin_reboot`, is enabled). Often, [systemd-pstore.service] is configured to
  carry out this task. This would conflict with `ticosd` performing this
  task. Therefore, [systemd-pstore.service] is automatically excluded when
  including the `meta-ticos` layer. Note that `ticosd` does not provide
  functionality (yet) to archive pstore files (like [systemd-pstore.service]
  can). If this is necessary for you, the work-around is to create a service
  that performs the archiving and runs before `ticosd.service` starts up.

### Changed

- [ticosd] When `ticosd` would fail to determine the reason for a reboot,
  it would assume that "low power" was reason for the reboot. This makes little
  sense because there are many resets for which `ticosd` is not able to
  determine a reason. This fallback is now changed to use "unspecified" in case
  the reason could not be determined (either from the built-in detection or
  externally, via the new `last_reboot_reason_file` API). Read the [new
  `last_reboot_reason_file` API][docs-reboots] for more information.
- Various improvements to the QEMU example integration:
  - It can now also be built for `qemuarm` (previously, only `qemuarm64` was
    working).
  - Linux pstore/ramoops subsystems are now correctly configured for the QEMU
    example integration, making it possible to test out the tracking of kernel
    panic reboot reasons using the QEMU device.
- [ticosd] The unit test set up is now run on `x86_64` as well as `i386` to
  get coverage on a 64-bit architecture as wel as a 32-bit one.

### Fixed

- [ticosd] Building the SDK on 32-bit systems would fail due to compilation
  errors. These are now fixed.
- [collectd] In the example, the statsd plugin would be listening on all network
  interfaces. This is narrowed to only listen on localhost (127.0.0.1).
- [ticosd] Many improvements to reboot reason tracking:
  - Intermittently, a reboot would erroneously be attributed to "low power".
  - Kernel panics would show up in the application as "brown out reset".
  - Sometimes, multiple reboot events for a single Linux reboot would get
    emitted. The root causes have been found and fixed. Logic has been added
    that tracks the Linux `boot_id` to ensure that at most one reboot reason
    gets emitted per Linux boot.
  - When using the example integration, the reboot reason "firmware update"
    would not be detected after SWUpdate had installed an OTA update. This was
    caused by a mismatch of the `defconfig` file in the example integration and
    the version of SWUpdate that was being compiled. This is now corrected.
- [ticosd] Fixed a bug in queue.c where an out-of-memory situation could lead
  to the queue's mutex not getting released.
- Improved the reliability of some of the E2E test scripts.

### Known Issues

- When `ticosd --enable-data-collection` is run and data collection had not
  yet been enabled, it will regenerate the SWUpdate configuration and restart
  the `swupdate.service`. This restart can cause SWUpdate to get into a bad
  state and fail to install OTA updates. This is not a new issue and was already
  present in previous releases. We are investigating this issue. As a
  work-around, the device can be rebooted immediately after running
  `ticosd --enable-data-collection`.
- The [systemd-pstore.service] gets disabled when including `meta-ticos`,
  even if `plugin_reboot` is disabled. As a work-around, if you need to keep
  [systemd-pstore.service], remove the `systemd_%.bbappend` file from the SDK.

[1.1.0]:
  https://github.com/ticos/observability-linux-sdk/releases/tag/1.1.0-kirkstone
[systemd-pstore.service]:
  https://www.freedesktop.org/software/systemd/man/systemd-pstore.service.html
[docs-reboots]: https://ticos.io/linux-reboots

## [1.0.0] - 2022-09-28

### Added

- This release is the first one including support for collecting and uploading
  user-land coredumps to the Ticos platform. The coredump plugin is enabled
  by default. Alongside this SDK release, an accompanying [Ticos
  CLI][docs-cli] version 0.11.0 aids in uploading symbol files to Ticos from
  Yocto builds to facilitate making use of the new functionality. Uploading
  symbols is a necessary step in order to use Ticos for coredumps. [Read more
  about coredump support in the Ticos Linux SDK][docs-coredumps].

[docs-coredumps]: https://ticos.io/linux-coredumps
[docs-cli]: https://ticos.io/ticos-cli

### Changed

- Breaking changes in the format of `/etc/ticosd.conf` (see [the updated
  reference][docs-reference-ticosd-conf]):
  - The `collectd` top-level key was merged into the `collectd_plugin` top-level
    key. The fields previously in `collectd` that have been moved to
    `collectd_plugin` are:
    - `interval_seconds`
    - `non_ticosd_chain`
    - `write_http_buffer_size_kib`
  - The `collectd_plugin.output_file` key has been replaced by two new keys:
    - `collectd_plugin.header_include_output_file`: the value of which should be
      included as the first statement in your `/etc/collectd.conf` file, and
    - `collectd_plugin.footer_include_output_file`: to be included as the last
      statement of your `/etc/collectd.conf` file.

[docs-reference-ticosd-conf]:
  https://docs.ticos.com/docs/linux/reference-ticosd-configuration/

### Fixed

- A misconfiguration bug whereby setting `collectd.interval_seconds` (now
  `collectd_plugin.interval_seconds`, see the "Changed" section of this release)
  would have no effect if our include file was at the bottom of
  `/etc/collectd.conf`. It happened due to the fact that collectd `Interval`
  statements are evaluated as they appear in source code (see [the author's
  statement][collectd-interval-eval]), only affecting the plugin statements that
  come after it.

[collectd-interval-eval]:
  https://github.com/collectd/collectd/issues/2444#issuecomment-331804766

### Known Issues

The server-side issue mentioned below has been resolved in the meantime.

~~Temporarily, our backend processing pipeline is unable to process coredumps
that link to shared objects in a specific style. This affects, in particular,
coredumps coming from devices on the Dunfell release of Yocto.~~

~~A backend fix has already been identified and should be released in the next
few business days. Once released, any previously collected coredumps that are
affected will be reprocessed server-side to address this issue. This will
**not** require any action from your team.~~

## [0.3.1] - 2022-09-05

### Added

- Support for Yocto version 3.1 (code name "Dunfell"). See the
  [`dunfell` branch](https://github.com/ticos/observability-linux-sdk/tree/dunfell)
  of the repository.

### Changed

- The SDK repository no longer has a `main` branch. The variant of the SDK that
  supports Yocto 4.0 ("Kirkstone") can be found on the
  [branch named `kirkstone`](https://github.com/ticos/observability-linux-sdk/tree/kirkstone).
  Likewise, the variant of the SDK that supports Yocto 3.1 ("Dunfell) can be
  found on
  [the branch called `dunfell`](https://github.com/ticos/observability-linux-sdk/tree/dunfell).

## [0.3.0] - 2022-08-31

### Added

- Initial support for collecting metrics using [collectd]. Check out the
  [docs on Metrics for Linux](https://ticos.io/linux-metrics) for more
  information.

[collectd]: https://collectd.org/

## [0.2.0] - 2022-08-10

This is our first public release. Head over to [our Linux
documentation][docs-linux] for an introduction to the Ticos Linux SDK.

[docs-linux]: https://docs.ticos.com/docs/linux/introduction

### Added

- [ticosd] Now implements exponential back-off for uploads. Requests
  originating from this exponential back-off system do not interfere with the
  regular upload interval.
- [ticosd] Sets persisted flag to disable data collection and returns
  immediately: `ticosd --disable-data-collection`.
- [ticosd] The `builtin.json` configuration file now features a link to
  documentation for reference.
- Improved the top-level `README.md` with a feature and architecture overview.

### Fixed

- [ticosd] The `--enable-data-collection` flag was not working reliably.
- [ticosd] A parsing bug going through the output of `ticos-device-info`.

### Known Issues

During start-up of the `ticosd` service, you may see a log line in the output
of `journalctl --unit ticosd`:

```
ticosd.service: Can't open PID file /run/ticosd.pid (yet?) after start: Operation not permitted
```

This file is only used by `systemd` during service shut-down and its absence
during start-up does not affect the functioning of the daemon. A fix is planned
for a future release. See [this report on the Ubuntu `nginx`
package][nginx-pid-report] for a discussion on the topic.

[nginx-pid-report]: https://bugs.launchpad.net/ubuntu/+source/nginx/+bug/1581864

## [0.1.0] - 2022-07-27

### Added

- [ticosd] Support reporting reboot reasons.
- [ticosd] Support OTA updates via SWUpdate.
- A ticosd layer for Yocto (meta-ticos).
- An example Yocto image using ticosd and the features above
  (meta-ticos-example).

[0.1.0]: https://github.com/ticos/observability-linux-sdk/releases/tag/0.1.0
[0.2.0]: https://github.com/ticos/observability-linux-sdk/releases/tag/0.2.0
[0.3.0]: https://github.com/ticos/observability-linux-sdk/releases/tag/0.3.0
[0.3.1]:
  https://github.com/ticos/observability-linux-sdk/releases/tag/0.3.1-kirkstone
[1.0.0]:
  https://github.com/ticos/observability-linux-sdk/releases/tag/1.0.0-kirkstone

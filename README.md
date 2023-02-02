# Ticos Linux SDK

Ship hardware products at the speed of software. With Ticos, you can
continuously monitor devices, debug firmware issues, and deploy OTA updates to
your fleet.

## Overview

The Ticos Linux SDK consists of a combination of existing open-source
software and a daemon [`ticosd`][source-ticosd] that orchestrates and
configures it. It also implements additional features such as tracking of reboot
reason tracking, and in the future reporting crashes and logs.

<p>
  <figure>
    <img
      src="/README-overview.svg"
      alt="Overview of the Ticos Linux SDK"
    />
    <figcaption>
      Dotted lines represent runtime configuration, and solid lines represent flow
      of data. Faded-out elements represent upcoming features.
    </figcaption>
  </figure>
</p>

To provide core Ticos platform features, the Ticos Linux SDK relies on
well-established, battle-tested open-source software. The daemon `ticosd`
implements Ticos-specific features and also acts as a configuration agent.

[source-ticosd]:
  https://github.com/ticos/observability-linux-sdk/blob/-/meta-ticos/recipes-ticos/ticosd/files/ticosd

## Prerequisites

Even though support for a broader diversity of setups is planned, this first
versions of our SDK makes the following assumptions:

- Your project uses [Yocto][yocto-homepage] as a build system.
- It uses [systemd][systemd-homepage] as an init system.
- It uses [SWUpdate][swupdate-homepage] for OTA (optional if you don't plan to
  integrate with OTA).

If your project diverges from these assumptions, please [get in
touch][get-in-touch]. It will likely still work without major changes.

[get-in-touch]: https://ticos.com/contact/

## Getting Started

Take a look at our [getting-started guide][docs-linux-getting-started] to set up
your integration.

OTA/Release Management is currently fully supported through an off-the-shelf
integration with the SWUpdate agent. Read more about it in the [OTA integration
guide][docs-linux-ota].

Metrics are also supported through [collectd][collectd-homepage]. Read more
about it in the [Linux Metrics integration guide][docs-linux-metrics].

[systemd-homepage]: https://systemd.io/
[swupdate-homepage]: https://swupdate.org/
[yocto-homepage]: https://www.yoctoproject.org/

## Documentation and Features

- Detailed documentation for the Ticos Linux SDK can be found in our online
  docs: see the [introduction][docs-linux-introduction] and the [getting-started
  guide][docs-linux-getting-started].
- Visit our [features overview][docs-platform] for a generic introduction to all
  the major features of the Ticos platform.

[docs-platform]: https://docs.ticos.com/docs/platform/introduction/
[docs-linux-introduction]: https://docs.ticos.com/docs/linux/introduction
[docs-linux-getting-started]: https://ticos.io/linux-getting-started

An integration example can be found under
[`/meta-ticos-example`](/meta-ticos-example). The central part of the SDK
lives in a Yocto layer in [`/meta-ticos`](/meta-ticos).

### OTA Updates

To provide OTA Updates, the Ticos Cloud implements an API endpoint compatible
with the [hawkBit DDI API][hawkbit-ddi]. Various clients are available, but
`ticosd` supports [SWUpdate][swupdate-homepage] out of the box and is able to
configure it to talk to our hawkBit DDI-compatible endpoint.

Read more about [Linux OTA management using Ticos][docs-linux-ota].

[docs-linux-ota]: https://ticos.io/linux-ota-integration-guide
[hawkbit-homepage]: https://www.eclipse.org/hawkbit/
[hawkbit-ddi]: https://www.eclipse.org/hawkbit/apis/ddi_api/
[swupdate-homepage]: https://swupdate.org/

### Metrics

The Ticos Linux SDK relies on [collectd][collectd-homepage] for the
collection and transmission of metrics. Application metrics can be sent to
collectd by means of [`statsd`][statsd-homepage].

Read more about [Linux metrics using Ticos][docs-linux-metrics].

[docs-linux-metrics]: https://ticos.io/linux-metrics
[collectd-homepage]: https://collectd.org/
[statsd-homepage]: https://github.com/statsd/statsd

### Crash Reports

To collect and upload user-land coredumps, the Ticos Linux SDK relies the
standard kernel [coredump][man-core] feature, and so does not need to make use
of any additional dependencies. Read more about [coredumps using the Ticos
Linux SDK][docs-linux-coredumps].

[docs-linux-coredumps]: https://ticos.io/linux-coredumps
[man-core]: https://man7.org/linux/man-pages/man5/core.5.html

### Reboot Reason Tracking and Logs

These features are fully in the domain of `ticosd`. Note that logs are an
upcoming feature, while reboot reason tracking and crash reports are supported
today. Read more about [reboot reason tracking using the Ticos Linux
SDK][docs-reboots].

[docs-reboots]: https://ticos.io/linux-reboots

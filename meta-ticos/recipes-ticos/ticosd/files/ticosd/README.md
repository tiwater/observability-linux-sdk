# `ticosd`

The `ticosd` service is the main orchestrator of the subsystems that conform
and are used by the Ticos Linux SDK. Among its responsibilities are:

- Keeping a queue of items to be uploaded to the Ticos cloud. This includes:
  - Reboot events.
  - User-land coredumps.
- Controlling whether data is collected from a device or not.
- Interacting and configuring subsystems such as SWUpdate and `collectd`.

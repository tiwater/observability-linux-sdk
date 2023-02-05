//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!

#include "ticos/util/reboot_reason.h"

bool ticosd_is_reboot_reason_valid(ticosRebootReason reboot_reason) {
  switch (reboot_reason) {
    case kTicosRebootReason_Unknown:
    case kTicosRebootReason_UserShutdown:
    case kTicosRebootReason_UserReset:
    case kTicosRebootReason_FirmwareUpdate:
    case kTicosRebootReason_LowPower:
    case kTicosRebootReason_DebuggerHalted:
    case kTicosRebootReason_ButtonReset:
    case kTicosRebootReason_PowerOnReset:
    case kTicosRebootReason_SoftwareReset:
    case kTicosRebootReason_DeepSleep:
    case kTicosRebootReason_PinReset:
    case kTicosRebootReason_UnknownError:
    case kTicosRebootReason_Assert:
    case kTicosRebootReason_WatchdogDeprecated:
    case kTicosRebootReason_BrownOutReset:
    case kTicosRebootReason_Nmi:
    case kTicosRebootReason_HardwareWatchdog:
    case kTicosRebootReason_SoftwareWatchdog:
    case kTicosRebootReason_ClockFailure:
    case kTicosRebootReason_KernelPanic:
    case kTicosRebootReason_FirmwareUpdateError:
    case kTicosRebootReason_BusFault:
    case kTicosRebootReason_MemFault:
    case kTicosRebootReason_UsageFault:
    case kTicosRebootReason_HardFault:
    case kTicosRebootReason_Lockup:
      return true;
    default:
      return false;
  }
}

const char *ticosd_reboot_reason_str(ticosRebootReason reboot_reason) {
  switch (reboot_reason) {
    case kTicosRebootReason_Unknown:
      return "Unknown";
    case kTicosRebootReason_UserShutdown:
      return "UserShutdown";
    case kTicosRebootReason_UserReset:
      return "UserReset";
    case kTicosRebootReason_FirmwareUpdate:
      return "FirmwareUpdate";
    case kTicosRebootReason_LowPower:
      return "LowPower";
    case kTicosRebootReason_DebuggerHalted:
      return "DebuggerHalted";
    case kTicosRebootReason_ButtonReset:
      return "ButtonReset";
    case kTicosRebootReason_PowerOnReset:
      return "PowerOnReset";
    case kTicosRebootReason_SoftwareReset:
      return "SoftwareReset";
    case kTicosRebootReason_DeepSleep:
      return "DeepSleep";
    case kTicosRebootReason_PinReset:
      return "PinReset";
    case kTicosRebootReason_UnknownError:
      return "UnknownError";
    case kTicosRebootReason_Assert:
      return "Assert";
    case kTicosRebootReason_WatchdogDeprecated:
      return "WatchdogDeprecated";
    case kTicosRebootReason_BrownOutReset:
      return "BrownOutReset";
    case kTicosRebootReason_Nmi:
      return "Nmi";
    case kTicosRebootReason_HardwareWatchdog:
      return "HardwareWatchdog";
    case kTicosRebootReason_SoftwareWatchdog:
      return "SoftwareWatchdog";
    case kTicosRebootReason_ClockFailure:
      return "ClockFailure";
    case kTicosRebootReason_KernelPanic:
      return "KernelPanic";
    case kTicosRebootReason_FirmwareUpdateError:
      return "FirmwareUpdateError";
    case kTicosRebootReason_BusFault:
      return "BusFault";
    case kTicosRebootReason_MemFault:
      return "MemFault";
    case kTicosRebootReason_UsageFault:
      return "UsageFault";
    case kTicosRebootReason_HardFault:
      return "HardFault";
    case kTicosRebootReason_Lockup:
      return "Lockup";
    default:
      return "???";
  }
}

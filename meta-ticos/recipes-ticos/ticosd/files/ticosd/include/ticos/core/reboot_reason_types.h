#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Different types describing information collected as part of "Trace Events"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TicosResetReason {
  kTicosRebootReason_Unknown = 0x0000,

  //
  // Normal Resets
  //

  kTicosRebootReason_UserShutdown = 0x0001,
  kTicosRebootReason_UserReset = 0x0002,
  kTicosRebootReason_FirmwareUpdate = 0x0003,
  kTicosRebootReason_LowPower = 0x0004,
  kTicosRebootReason_DebuggerHalted = 0x0005,
  kTicosRebootReason_ButtonReset = 0x0006,
  kTicosRebootReason_PowerOnReset = 0x0007,
  kTicosRebootReason_SoftwareReset = 0x0008,

  // MCU went through a full reboot due to exit from lowest power state
  kTicosRebootReason_DeepSleep = 0x0009,
  // MCU reset pin was toggled
  kTicosRebootReason_PinReset = 0x000A,

  //
  // Error Resets
  //

  // Can be used to flag an unexpected reset path. i.e NVIC_SystemReset()
  // being called without any reboot logic getting invoked.
  kTicosRebootReason_UnknownError = 0x8000,
  kTicosRebootReason_Assert = 0x8001,

  // Deprecated in favor of HardwareWatchdog & SoftwareWatchdog. This way,
  // the amount of watchdogs not caught by software can be easily tracked
  kTicosRebootReason_WatchdogDeprecated = 0x8002,

  kTicosRebootReason_BrownOutReset = 0x8003,
  kTicosRebootReason_Nmi = 0x8004,  // Non-Maskable Interrupt

  // More details about nomenclature in https://ticos.io/root-cause-watchdogs
  kTicosRebootReason_HardwareWatchdog = 0x8005,
  kTicosRebootReason_SoftwareWatchdog = 0x8006,

  // A reset triggered due to the MCU losing a stable clock. This can happen,
  // for example, if power to the clock is cut or the lock for the PLL is lost.
  kTicosRebootReason_ClockFailure = 0x8007,

  // A software reset triggered when the OS or RTOS end-user code is running on top of identifies
  // a fatal error condition.
  kTicosRebootReason_KernelPanic = 0x8008,

  // A reset triggered when an attempt to upgrade to a new OTA image has failed and a rollback
  // to a previous version was initiated
  kTicosRebootReason_FirmwareUpdateError = 0x8009,

  // Resets from Arm Faults
  kTicosRebootReason_BusFault = 0x9100,
  kTicosRebootReason_MemFault = 0x9200,
  kTicosRebootReason_UsageFault = 0x9300,
  kTicosRebootReason_HardFault = 0x9400,
  // A reset which is triggered when the processor faults while already
  // executing from a fault handler.
  kTicosRebootReason_Lockup = 0x9401,
} eTicosRebootReason;

#ifdef __cplusplus
}
#endif

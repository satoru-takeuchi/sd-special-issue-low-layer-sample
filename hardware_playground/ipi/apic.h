#pragma once
#include <stdint.h>
#include "misc.h"

extern int apic_version;

static inline uint32_t get_apicid() {
  // see intel SDM vol.3 10.4.6 Local APIC ID
  switch(apic_version) {
  case 1:
    {
      static uint32_t * const kIdReg = (uint32_t *)0xFEE00020;
      return *kIdReg >> 24;
    }
  case 2:
    {
      static const uint32_t kIdReg = 0x802;
      return rdmsr(kIdReg);
    }
  }
  return 0xFFFFFFFF;
}

static inline void send_ipi(uint32_t destid, uint8_t vector) {
  static const uint32_t kIcrRegTriggerModeLevel = 1 << 15;
  static const uint32_t kIcrRegLevelAssert = 1 << 14;
  switch(apic_version) {
  case 1:
    {
      // see intel SDM vol.3 10.6.1 Interrupt Command Register (ICR)
      static uint32_t * const kIcrLowReg = (uint32_t *)0xFEE00300;
      static uint32_t * const kIcrHighReg = (uint32_t *)0xFEE00310;
      *kIcrHighReg = (destid & 0xFF) << 24;
      *kIcrLowReg = kIcrRegTriggerModeLevel | kIcrRegLevelAssert | vector;
      break;
    }
  case 2:
    {
      // see intel SDM vol.3 10.12.9 ICR Operation in x2APIC Mode
      static const uint32_t kIcrReg = 0x830;
      wrmsr(kIcrReg, ((uint64_t)destid << 32) | kIcrRegTriggerModeLevel | kIcrRegLevelAssert | vector);
      break;
    }
  }
}

static inline void send_eoi() {
  // see Intel SDM vol.3 10.8.5 Signaling Interrupt Servicing Completion
  switch(apic_version) {
  case 1:
    {
      static uint32_t * const kEoiReg = (uint32_t *)0xFEE000B0;
      *kEoiReg = 0;
      break;
    }
  case 2:
    {
      static const uint32_t kEoiReg = 0x80B;
      wrmsr(kEoiReg, 0);
      break;
    }
  }
}

static inline int localapic_init(uint8_t error_vector) {
  // see Intel SDM vol.3 10.4.3 Enabling or Disabling the Local APIC
  static const uint32_t kIa32ApicBaseMsr = 0x1B;
  uint64_t msr = rdmsr(kIa32ApicBaseMsr);
  static const uint32_t kApicGlobalEnableFlag = 1 << 11;
  static const uint32_t kX2ApicEnableFlag = 1 << 10;
  if (!(msr & kApicGlobalEnableFlag)) {
    return -1;
  }
  if (msr & kX2ApicEnableFlag) {
    apic_version = 2;
  }

  static const uint32_t kSvrRegApicSoftwareEnableFlag = 1 << 8;
  switch(apic_version) {
  case 1:
    {
      static uint32_t * const kSvrReg = (uint32_t *)0xFEE000F0;
      *kSvrReg = kSvrRegApicSoftwareEnableFlag | error_vector;
      break;
    }
  case 2:
    {
      static const uint32_t kSvrReg = 0x80F;
      wrmsr(kSvrReg, kSvrRegApicSoftwareEnableFlag | error_vector);
      break;
    }
  }
  return 0;
}

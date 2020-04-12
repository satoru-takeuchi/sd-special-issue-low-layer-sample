#pragma once
#include <stdint.h>

static inline uint64_t rdmsr(uint32_t addr) {
  uint32_t msr_high, msr_low;
  asm volatile("rdmsr" : "=d"(msr_high), "=a"(msr_low) : "c"(addr));
  return (static_cast<uint64_t>(msr_high) << 32) | msr_low;
}

static inline void wrmsr(uint32_t addr, uint64_t data) {
  uint32_t msr_high = (data >> 32) & 0xffffffff;
  uint32_t msr_low = data & 0xffffffff;
  asm volatile("wrmsr" ::"d"(msr_high), "a"(msr_low), "c"(addr));
}

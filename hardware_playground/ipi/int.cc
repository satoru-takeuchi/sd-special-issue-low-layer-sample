#include "int.h"
#include <stdint.h>

typedef uint64_t virt_addr;

volatile uint16_t _idtr[5];

static const int kIdtEntryMax = 64;
struct idt_entity {
  uint32_t entry[4];
} __attribute__((aligned(8))) idt_def[kIdtEntryMax];

int setup_inthandler(int index, void (*handler)()) {
  if (index >= kIdtEntryMax) {
    return -1;
  }
  virt_addr vaddr = reinterpret_cast<virt_addr>(handler);
  static const uint8_t kKernelCodeSegment = 0x10;
  idt_def[index].entry[0] = (vaddr & 0xFFFF) | (kKernelCodeSegment << 16);
  idt_def[index].entry[1] = (vaddr & 0xFFFF0000) | (0xE << 8) | (1 << 15) | (3 << 13);
  idt_def[index].entry[2] = vaddr >> 32;
  idt_def[index].entry[3] = 0;

  virt_addr idt_addr = reinterpret_cast<virt_addr>(idt_def);
  _idtr[0] = 0x10 * kIdtEntryMax - 1;
  _idtr[1] = idt_addr & 0xffff;
  _idtr[2] = (idt_addr >> 16) & 0xffff;
  _idtr[3] = (idt_addr >> 32) & 0xffff;
  _idtr[4] = (idt_addr >> 48) & 0xffff;
  return 0;
}

void setup_idt() {
  asm volatile("lidt (%0)" ::"r"(_idtr));
}

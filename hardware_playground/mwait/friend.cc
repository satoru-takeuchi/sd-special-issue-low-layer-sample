#include <toshokan/friend/offload.h>
#include "shared.h"

int core_cnt = 0;
uint64_t SHARED_SYMBOL(main_sync) = 0;

bool is_monitor_available() {
  // check the availability of MONITOR instruction by CPUID.01H:ECX.MONITOR[bit 3]
  uint32_t ecx;
  asm volatile("cpuid":"=c"(ecx):"a"(0x1):"ebx","edx");
  return (ecx & (1 << 3));
}

void friend_main() {
  int current_core_id = __sync_fetch_and_add(&core_cnt, 1);

  if (!is_monitor_available()) {
    OFFLOAD({
	EXPORTED_SYMBOL(printf)("address monitoring hardware is not available\n");
      });
    return;
  }
  
  asm volatile("monitor"::"a"(&SHARED_SYMBOL(main_sync)), "c"(0), "d"(0));

  // unsynchronized incrementation
  // must be done with an atomic operation
  __sync_fetch_and_add(&SHARED_SYMBOL(main_sync), 1);

  // wait until hakase increments the variable
  if (current_core_id == 0) {
    while(SHARED_SYMBOL(main_sync) < kCoreCount + 1) {
      asm volatile("pause":::"memory");
    }
  }
  // if current_core_id is 1, invoke mwait immediately

  // synchonized countup
  for(uint64_t i = 0; ; i++) {
    if ((int)(i % 2) == current_core_id) {
      SHARED_SYMBOL(main_sync)++;
    } else {
      while(i + kCoreCount + 2 != SHARED_SYMBOL(main_sync)) {
	asm volatile("mwait"::"a"(0),"c"(0):"memory");
      }
    }
  }
}

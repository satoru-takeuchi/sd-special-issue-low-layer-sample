#include <toshokan/friend/offload.h>
#include "shared.h"

int core_cnt = 0;
uint64_t SHARED_SYMBOL(main_sync) = 0;

void friend_main() {
  int current_core_id = __sync_fetch_and_add(&core_cnt, 1);

  // unsynchronized incrementation
  // must be done with an atomic operation
  __sync_fetch_and_add(&SHARED_SYMBOL(main_sync), 1);

  // wait until hakase increments the variable
  while(SHARED_SYMBOL(main_sync) < kCoreCount + 1) {
    asm volatile("pause":::"memory");
  }

  // synchonized countup
  for(uint64_t i = 0; ; i++) {
    if ((int)(i % 2) == current_core_id) {
      while(i + kCoreCount + 1 != __sync_fetch_and_add(&SHARED_SYMBOL(main_sync), 0)) {
	asm volatile("pause":::"memory");
      }
      __sync_fetch_and_add(&SHARED_SYMBOL(main_sync), 1);
    }
  }
}

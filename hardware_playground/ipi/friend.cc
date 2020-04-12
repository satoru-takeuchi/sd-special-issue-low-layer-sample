#include <toshokan/friend/offload.h>
#include "shared.h"
#include "int.h"
#include "apic.h"

int core_cnt = 0;
uint64_t SHARED_SYMBOL(main_sync) = 0;
uint32_t apicid_list[2];
int apic_version = 1;

extern "C" void int_handler(); // defined at int_asm.S

struct Page {
  uint64_t entry[512];
} __attribute__((aligned(4096)));

extern Page SHARED_SYMBOL(__toshokan_pdpt);
Page toshokan_pd;

void friend_main() {
  int current_core_id = __sync_fetch_and_add(&core_cnt, 1);
  asm volatile("cli;");
  
  static const uint8_t kIpiVector = 32;
  static const uint8_t kApicErrorVector = 33;
  if (setup_inthandler((int)kIpiVector, int_handler) ||
      setup_inthandler((int)kApicErrorVector, int_handler)) {
    OFFLOAD({
	EXPORTED_SYMBOL(printf)("failed to initialize int handlers\n");
      });
    return;
  }
  setup_idt();

  // map latter 3GB-4GB to access memory maped IPI register
  static const uint64_t kPteFlags = (1 << 0) | (1 << 1) | (1 << 2);
  static const uint64_t kPteFlagsForHugePages = kPteFlags | (1 << 7);
  SHARED_SYMBOL(__toshokan_pdpt).entry[3] = reinterpret_cast<uint64_t>(&toshokan_pd) | kPteFlags;
  for (int i = 0; i < 512; i++) {
    toshokan_pd.entry[i] = (i * 0x200000 /* 2MB */ + 0xc0000000 /* 3GB */) | kPteFlagsForHugePages;
  }
  
  if (localapic_init(kApicErrorVector) < 0) {
    OFFLOAD({
	EXPORTED_SYMBOL(printf)("failed to initialize localapic\n");
      });
    return;
  }

  uint32_t apicid = get_apicid();
  apicid_list[current_core_id] = apicid;
  OFFLOAD({
      EXPORTED_SYMBOL(printf)("apic id of core %d is %d\n", current_core_id, apicid);
    });

  // unsynchronized incrementation
  // must be done with an atomic operation
  __sync_fetch_and_add(&SHARED_SYMBOL(main_sync), 1);

  // synchonized countup
  for(uint64_t i = 0; ; i++) {
    if ((int)(i % 2) == current_core_id) {
      while(SHARED_SYMBOL(main_sync) != i + kCoreCount + 1) {
	asm volatile("pause":::"memory");
      }
      // invoke IPI
      int target_core_id = current_core_id ^ 1;
      send_ipi(apicid_list[target_core_id], kIpiVector);
      SHARED_SYMBOL(main_sync)++;
    } else {
      // this HLT will be woken up by IPI
      // IPI is handled by int_handler, which do nothing.
      asm volatile("sti;hlt;cli;");
      send_eoi();
    }
  }
}

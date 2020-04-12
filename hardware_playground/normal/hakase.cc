#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <toshokan/hakase/hakase.h>
#include <unistd.h>
#include <thread>
#include "shared.h"

void measurement_main(bool &finished) {
  // initial sync between hakase and friend
  while(!is_friend_stopped() && (SHARED_SYMBOL(main_sync) != kCoreCount)) {
    asm volatile("pause":::"memory");
  }
  
  printf("measuring synchronization...\n");
  
  struct timeval t1;
  struct timeval t2;
  
  SHARED_SYMBOL(main_sync)++; // friends are waiting until hakase increments the variable
  gettimeofday(&t1, NULL);

  // wait 10 seconds
  while(true) {
    if (is_friend_stopped()) {
      return;
    }
    gettimeofday(&t2, NULL);
    if (t2.tv_sec - t1.tv_sec >= 10) {
      break;
    }
    usleep(10000);
  }

  uint64_t count = __sync_fetch_and_add(&SHARED_SYMBOL(main_sync), 0);
  uint64_t time = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)) * 1000;
  time /= count;
  printf("count: %ld\nsync cost:%ldns\n", count, time);
  finished = true;
}

int test_main() {
  int r;
  r = setup();
  if (r != 0) {
    return 0;
  }

  printf("starting up friend cores...");
  fflush(stdout);
  if (boot(kCoreCount) != kCoreCount) {
    printf("failed\n");
    return 0;
  }
  printf("done\n");
  fflush(stdout);

  bool subthread_finished = false;
  std::thread t(measurement_main, std::ref(subthread_finished));

  while(!subthread_finished && !is_friend_stopped()) {
    offloader_tryreceive();
    usleep(10000);
  }
  t.join();
  return 1;
}

int main(int argc, const char **argv) {
  if (test_main() > 0) {
    printf("\e[32m%s: COMPLETED\e[m\n", argv[0]);
    return 0;
  } else {
    printf("\e[31m%s: FAILED\e[m\n", argv[0]);
    return 255;
  }
}

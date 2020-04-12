#include <sys/time.h>
#include <semaphore.h>
#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <thread>
#include <sched.h>

int thread_cnt = 0;
uint64_t main_sync = 0;
int finished = 0;
static const int kThreadCount = 2;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void sync_main() {
  int current_thread_id = __sync_fetch_and_add(&thread_cnt, 1);

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(current_thread_id + 1, &cpu_set);
  
  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) < 0) {
    perror("failed to set sched_setaffinity");
  }

  // unsynchronized incrementation
  // must be done with an atomic operation
  __sync_fetch_and_add(&main_sync, 1);

  if (current_thread_id == 0) {
    while(main_sync != kThreadCount + 1) {
      asm volatile("pause":::"memory");
    }
  }
  
  pthread_mutex_lock(&mtx);
  // synchonized countup
  for(uint64_t i = 0; ; i++) {
    if ((int)(i % 2) == current_thread_id) {
      pthread_cond_signal(&cond);
      main_sync++;
    } else {
      pthread_cond_wait(&cond, &mtx);
      if (finished) {
	pthread_mutex_unlock(&mtx);
	return;
      }
    }
  }
}


void measurement_main() {
  while(main_sync != kThreadCount) {
    asm volatile("pause":::"memory");
  }
  
  printf("measuring synchronization...\n");
  
  struct timeval t1;
  struct timeval t2;
  
  main_sync++; // friends are waiting until hakase increments the variable
  gettimeofday(&t1, NULL);

  // wait 10 seconds
  while(true) {
    gettimeofday(&t2, NULL);
    if (t2.tv_sec - t1.tv_sec >= 10) {
      break;
    }
    usleep(10000);
  }

  uint64_t count = __sync_fetch_and_add(&main_sync, 0);
  uint64_t time = (uint64_t)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)) * 1000;
  time /= count;
  printf("count: %ld\nsync cost:%ldns\n", count, time);
  finished = 1;
}

int main() {
  std::thread m_th(measurement_main);
  std::thread th1(sync_main);
  std::thread th2(sync_main);
  m_th.join();
  pthread_cond_broadcast(&cond);
  th1.join();
  th2.join();
  return 0;
}


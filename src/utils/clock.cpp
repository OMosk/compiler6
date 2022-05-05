#include "clock.h"

#include <time.h>

struct timespec start_clock;
void initClock() {
  clock_gettime(CLOCK_MONOTONIC, &start_clock);
}

double now() {
  struct timespec res;
  //TODO @LinuxPortability replace compile time clock_gettime with dlopen/dlget to
  //  to avoid linking with newer symbols from new GLIBC
  clock_gettime(CLOCK_MONOTONIC, &res);
  res.tv_sec -= start_clock.tv_sec;
  res.tv_nsec -= start_clock.tv_nsec;
  if (res.tv_nsec < 0) {
    res.tv_sec -= 1;
    res.tv_nsec += 1000000000;
  }
  time_t sec = res.tv_sec;
  long nsec = res.tv_nsec;
  return (sec * 1.0) + (nsec * 1e-9);
}

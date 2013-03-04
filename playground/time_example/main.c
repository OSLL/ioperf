#include <stdio.h>
#include <unistd.h>
#include <time.h>

typedef unsigned long long ticks;

static __inline__ ticks getticks(void) {
   unsigned a, d;
   __asm__ ("lfence");
   __asm__ volatile("rdtscp" : "=a"(a), "=d"(d));
   return (((ticks)a) | (((ticks)d) << 32));
}

void benchTSC(int loopN, int usleepN, struct timespec nanosleepN) {
   ticks ticksBegin;
   ticks ticksEnd;
   ticksBegin = getticks();
   for(int i = 0; i < loopN; ++i);
   ticksEnd = getticks();
   printf("TSC: loop of %d in %llu\n", loopN, (ticksEnd - ticksBegin));

   ticksBegin = getticks();
   usleep(usleepN);
   ticksEnd = getticks();
   printf("TSC: usleep of %d microseconds in %llu\n", usleepN, (ticksEnd - ticksBegin));

   ticksBegin = getticks();
   nanosleep(&nanosleepN, 0);
   ticksEnd = getticks();
   printf("TSC: nanosleep of %g seconds %ld nanoseconds in %llu\n",
         difftime(nanosleepN.tv_sec, 0), nanosleepN.tv_nsec, ticksEnd - ticksBegin);
}

void benchHPET(int loopN, int usleepN, struct timespec nanosleepN) {
   struct timespec ticksBegin;
   struct timespec ticksEnd;
   clock_gettime(CLOCK_MONOTONIC, &ticksBegin);
   for(int i = 0; i < loopN; ++i);
   clock_gettime(CLOCK_MONOTONIC, &ticksEnd);
   printf("HPET: loop of %d in %g seconds %ld nanoseconds\n", loopN,
         difftime(ticksEnd.tv_sec, ticksBegin.tv_sec),
         ticksEnd.tv_nsec - ticksBegin.tv_nsec);

   clock_gettime(CLOCK_MONOTONIC, &ticksBegin);
   usleep(usleepN);
   clock_gettime(CLOCK_MONOTONIC, &ticksEnd);
   printf("HPET: usleep of %d microseconds in %g seconds %ld nanoseconds\n", usleepN,
         difftime(ticksEnd.tv_sec, ticksBegin.tv_sec),
         ticksEnd.tv_nsec - ticksBegin.tv_nsec);

   clock_gettime(CLOCK_MONOTONIC, &ticksBegin);
   nanosleep(&nanosleepN, 0);
   clock_gettime(CLOCK_MONOTONIC, &ticksEnd);
   printf("HPET: nanosleep of %g seconds %ld nanoseconds in %g seconds %ld nanoseconds\n",
         difftime(nanosleepN.tv_sec, 0), nanosleepN.tv_nsec,
         difftime(ticksEnd.tv_sec, ticksBegin.tv_sec),
         ticksEnd.tv_nsec - ticksBegin.tv_nsec);
}


int main() {
   int loopN = 10000000;
   int usleepN = 50000;
   struct timespec nanosleepN = { .tv_sec = 0, .tv_nsec = 1000 };
   benchHPET(loopN, usleepN, nanosleepN);
   benchTSC(loopN, usleepN, nanosleepN);
}

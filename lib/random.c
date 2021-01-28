#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "random.h"




double current_time_millisecond() {
struct timeval  tv;
gettimeofday(&tv, NULL);
double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecond

return time_in_mill;
}



int randomprodotti(int P) {	
    unsigned int seed=(unsigned int)current_time_millisecond()+(unsigned int) pthread_self();
    int tmp;
    tmp=rand_r(&seed);
    tmp=tmp%P;

    return tmp;
}



int randomtimeacquisticliente(int T) {
    unsigned int seed=(unsigned int)current_time_millisecond()+(unsigned int) pthread_self();
    int mintime=10;
    int tmp=rand_r(&seed);
    tmp=tmp%T;
    if (tmp<mintime)
        tmp=tmp+mintime;

    return tmp;
}



int generatempofissocassiere(long var)  {
    int mintime=20; int maxtime=80;
    int tmp=0;
    unsigned int seed=(unsigned int)current_time_millisecond()*var;
    seed=seed+(unsigned int) pthread_self();
    tmp=rand_r (&seed);
    tmp=tmp%(maxtime+1);
    if (tmp<mintime)
        tmp=tmp+mintime;

    return tmp+var;
}



int ms_sleep(unsigned int ms)   {
  int result = 0;

  {
    struct timespec ts_remaining = { 
      ms / 1000, 
      (ms % 1000) * 1000000L 
    };

    do {
      struct timespec ts_sleep = ts_remaining;
      result = nanosleep(&ts_sleep, &ts_remaining);
    } 
    while ((EINTR == errno) && (-1 == result));
  }
  if (-1 == result) {
    perror("nanosleep() failed");
  }

  return result;
}

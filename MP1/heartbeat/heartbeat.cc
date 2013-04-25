#include "heartbeat.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

static const int ONE_MILLION = 1000000L;

static void* send_thread_main(void *arg) {
  Heartbeat* heartbeat = (Heartbeat*) arg;

  timespec req;
  req.tv_sec = (heartbeat->getSendInterval() / 1000); 
  req.tv_nsec = (heartbeat->getSendInterval() % 1000) * ONE_MILLION; 
  while(1) {
    nanosleep(&req, NULL);
    heartbeat->sendCallback();
  }
}

Heartbeat::Heartbeat(
    int id,
    int* memberIds,
    int memberCount,
    long timeout,
    long sendInterval,
    void (*failureHandler)(int, siginfo_t*, void *), 
    void (*sendCallback)(void))
    : sendCallback(sendCallback), sendInterval(sendInterval) {

  timerSpec.it_interval.tv_sec = (timeout / 1000); 
  timerSpec.it_interval.tv_nsec = (timeout % 1000) * ONE_MILLION;
  timerSpec.it_value.tv_sec = (timeout / 1000);
  timerSpec.it_value.tv_nsec = (timeout % 1000) * ONE_MILLION;

  if (pthread_create(&sendThread, NULL, &send_thread_main, (void*) this) != 0) {
    fprintf(stderr, "Error creating heartbeat send thread!\n");
    exit(1);
  }

  for (int i = 0; i < memberCount; ++i) {  
    // Only set timers for external nodes.
    if (id != memberIds[i]) {  
      // Set the timer callback.
      struct sigaction sa; 
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_SIGINFO; 
      sa.sa_sigaction = failureHandler;
      
      if (sigaction(SIGRTMAX, &sa, NULL) == -1) {
        fprintf(stderr, "Error setting heartbeat listen signal callback!\n");
        exit(1);
      }

      // Create heartbeat timers.
      timer_t timer;
      sigevent sigev;

      sigev.sigev_notify = SIGEV_SIGNAL;
      sigev.sigev_signo = SIGRTMAX;
      sigev.sigev_value.sival_int = memberIds[i];

      if (timer_create(CLOCK_MONOTONIC, &sigev, &timer) == -1) {
        fprintf(stderr, "Error creating heartbeat listen timer!\n");
        exit(1);
      }
      listenTimers[memberIds[i]] = timer;
    }
  }
}

void Heartbeat::arm() {
  for (
      map<int, timer_t>::iterator it = listenTimers.begin();
      it != listenTimers.end();
      it++) {

    timer_t timer = it->second;
    if (timer_settime(timer, 0, &timerSpec, NULL) != 0) {
      fprintf(stderr, "Error arming heartbeat listen timers!\n");
      exit(1);
    }
  }
}

void Heartbeat::reset(int id) {
  timer_t timer = listenTimers[id];
  if (timer_settime(timer, 0, &timerSpec, NULL) != 0) {
    fprintf(stderr, "Error reseting heartbeat listen timers!\n");
    exit(1);
  }
}



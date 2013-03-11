#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <map>

using namespace std;

struct Heartbeat {
  private:
    pthread_t sendThread;

    struct itimerspec timerInterval;
    map<int, timer_t> listenTimers;

  public:   
    void (*sendCallback)(void);

    Heartbeat(
        int* memberIds, 
        int memberCount, 
        int interval, 
        void (*failureHandler)(int, siginfo_t*, void *),
        void (*sendCallback)(void));
    
    void arm();
    void reset(int id);
};

#endif

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

    long sendInterval;
    struct itimerspec timerSpec;
    map<int, timer_t> listenTimers;

  public:   
    void (*sendCallback)(void);

    Heartbeat(
        int id,
        int* memberIds, 
        int memberCount, 
        long timeout,
        long sendInterval,
        void (*failureHandler)(int, siginfo_t*, void *),
        void (*sendCallback)(void));
    
    void arm();
    void reset(int id);

    long getSendInterval() { return sendInterval; }
};

#endif

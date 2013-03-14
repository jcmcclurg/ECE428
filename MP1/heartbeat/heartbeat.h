#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <map>

using namespace std;

//! Represents a heartbeat failure detector.
struct Heartbeat {
  private:
    //! A thread that sends a heartbeat to all processes in the group every sendInteral ms.
    pthread_t sendThread;

    //! The time between heartbeats in ms.
    long sendInterval;

    //! The itimerspec we want to reset our timers to after receiving a heartbeat.
    //! The conversion from timeout to this struct is done in the constructor.
    struct itimerspec timerSpec;

    //! A map from process ID to a heartbeat receive timer.
    map<int, timer_t> listenTimers;

  public: 
    //! A callback to execute every interval inside the send thread.
    void (*sendCallback)(void);

    //! Constructs a Heartbeat with a process ID, an array of group members, a
    //! timeout, a send interval, and callbacks to execute upon failure or
    //! heartbeat send.
    Heartbeat(
        int id,
        int* memberIds, 
        int memberCount, 
        long timeout,
        long sendInterval,
        void (*failureHandler)(int, siginfo_t*, void *),
        void (*sendCallback)(void));
    
    long getSendInterval() { return sendInterval; }

    //! Signals all the heartbeat receive timers to start counting down.
    void arm();

    //! Reset the timeout for the timer with the provided ID.
    //!
    //! @param id The process ID of the timer we want to reset.
    void reset(int id);

};

#endif

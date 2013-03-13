#ifndef __RMCAST_H_
#define __RMCAST_H_

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <set>
#include <algorithm>
#include <ctime>
#include <stdlib.h>
#include <sstream>

using namespace std;

#include "operators.h"
#include "heartbeat/heartbeat.h"
#include "message/message.h"
#include "state/state.h"
#include "timestamp/timestamp.h"
#include "mp1.h"
#include "rmcast.h"

/* Defines */
#define HEARTBEAT_MS 0.2 * 1000L

void populateAcknowledgements(map<int, int>& acknowledgements);

void unicast(int to, Message& m);

static void heartbeat_failure(int sig, siginfo_t *si, void *uc);
static void heartbeat_send();

void multicast_deliver(Message& m);
void initIfNecessary();
void discard(Message* m);

#endif

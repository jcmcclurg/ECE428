#include <iostream>
#include <string>
#include <assert.h>

#include "heartbeat/heartbeat.h"
#include "state/state.h"
#include "message/message.h"
#include "timestamp/timestamp.h"

void testSerialize() {
  NodeState state(1);
  vector<pair<int, int> > acknowledgements;
  acknowledgements.push_back(make_pair(2, 1));

  Message* a = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    MESSAGE,
    string("hey"),
    acknowledgements
  );

  string str = a->getEncodedMessage();
  Message b = Message(str);

  assert(b.getSenderId() == state.getId());
  assert(b.getMessage() == "hey");
  assert(acknowledgements.size() == 1);

  delete a;
}

void s() {
  std::cout << "hey" << std::endl;
}

void f(int sig, siginfo_t *si, void *uc) {
  std::cout << si->si_value.sival_int << std::endl;
}

void testHeartbeat() {
  int ids[] = { 3, 4, 5 };
  Heartbeat heartbeat(ids, 3, 500, f, s);
  heartbeat.arm();
  while(1) {
    heartbeat.reset(3);
  }
}

int main() {
  testSerialize();
  //testHeartbeat();
}
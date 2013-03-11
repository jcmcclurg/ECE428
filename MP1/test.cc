#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <set>

#include "heartbeat/heartbeat.h"
#include "state/state.h"
#include "message/message.h"
#include "timestamp/timestamp.h"

void testSerialize() {
  vector<int> v;v.push_back(1);v.push_back(2);v.push_back(3);
  map<int, int> acknowledgements;
  set<int> failedNodes;
  acknowledgements[2] = 1;

  NodeState state(1, v);

  Message* a = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    MESSAGE,
    string("hey"),
    acknowledgements,
    failedNodes
  );

  string str = a->getEncodedMessage();
  Message b = Message(str);

  assert(b.getSenderId() == state.getId());
  assert(b.getMessage() == "hey");
  assert(acknowledgements.size() == 1);

  delete a;
}

void testMessage(){
  vector<int> v;v.push_back(1);v.push_back(2);v.push_back(3);
  map<int, int> acknowledgements;
  set<int> failedNodes;
  acknowledgements[2] = 1;

  Timestamp ts1(1,v);
  ts1.step();

  Timestamp ts2(2,v);
  ts2.step();

  Timestamp ts3(2,v);
  ts3.update(ts1);
  ts3.update(ts2);

  Message a(1,1,ts1,MESSAGE,string("hey"),acknowledgements, failedNodes);
  Message b(2,2,ts2,MESSAGE,string("hey"),acknowledgements, failedNodes);
  Message c(3,3,ts3,MESSAGE,string("hey"),acknowledgements, failedNodes);

  assert(b < c);
  assert(a < c);
  assert(a < b);

  vector<Message> msgs;
  msgs.push_back(b);
  msgs.push_back(c);
  msgs.push_back(a);

  sort(msgs.begin(), msgs.end());

  for (vector<Message>::iterator it=msgs.begin(); it!=msgs.end(); ++it) {
    std::cout << ' ' << (*it).getSenderId();
  }
  std::cout << '\n';
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
  testMessage();
  //testHeartbeat();
}

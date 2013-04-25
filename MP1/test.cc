#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <set>

#include "heartbeat/heartbeat.h"
#include "state/state.h"
#include "message/message.h"
#include "timestamp/timestamp.h"
#include "rmcast.h"
#include "mp1.h"

void testSerialize() {
  set<int> v; v.insert(1); v.insert(2); v.insert(3);
  map<int, int> acknowledgements;
  set<int> failedNodes;
  acknowledgements[2] = 1;

  Node state(1, v);

  Message* a = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    MESSAGE,
    string("hey"),
    acknowledgements,
    failedNodes
  );

  Message* b = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    RETRANSREQUEST,
    string("1"),
    acknowledgements,
    failedNodes
  );

  Message* c = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    HEARTBEAT,
    string(),
    acknowledgements,
    failedNodes
  );

  char buf[1000];

  int len = a->getEncodedMessage(buf);
  Message a1 = Message(buf, len);

  assert(a1.getSenderId() == state.getId());
  assert(a1.getMessage() == "hey");
  assert(a1.getTimestamp().getTimestampMap()[1] == 0);
  assert(a1.getAcknowledgements()[2] == 1);
  assert(a1.getFailedNodes().size() == 0);

  len = b->getEncodedMessage(buf);
  Message b1 = Message(buf, len);

  assert(b1.getSenderId() == state.getId());
  assert(b1.getMessage() == "1");
  assert(b1.getTimestamp().getTimestampMap()[1] == 0);
  assert(b1.getAcknowledgements()[2] == 1);
  assert(b1.getFailedNodes().size() == 0);

  len = c->getEncodedMessage(buf);
  Message c1 = Message(buf, len);

  assert(c1.getSenderId() == state.getId());
  assert(c1.getTimestamp().getTimestampMap()[1] == 0);
  assert(c1.getAcknowledgements()[2] == 1);
  assert(c1.getFailedNodes().size() == 0);

  delete a;
  delete b;
  delete c;
}

void testMessage(){
  set<int> v; v.insert(1); v.insert(2); v.insert(3);
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
  std::cout << HEARTBEAT_MS << ":" << HEARTBEAT_MS + MAXDELAY / 1000L << std::endl;
  Heartbeat heartbeat(ids, 3, HEARTBEAT_MS + MAXDELAY / 1000L, HEARTBEAT_MS, f, s);
  heartbeat.arm();
  while(1) {
    heartbeat.reset(3);
  }
}

int main() {
  //testSerialize();
  //testMessage();
  testHeartbeat();
}

#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

#include "heartbeat/heartbeat.h"
#include "state/state.h"
#include "message/message.h"
#include "timestamp/timestamp.h"

void testSerialize() {
  vector<int> v;v.push_back(1);v.push_back(2);v.push_back(3);
  NodeState state(1,v);
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

void testMessage(){
  vector<int> v;v.push_back(1);v.push_back(2);v.push_back(3);
  vector<pair<int, int> > acknowledgements;
  acknowledgements.push_back(make_pair(2, 1));

  Timestamp ts1(1,v);
  ts1.step();

  Timestamp ts2(2,v);
  ts2.step();

  Timestamp ts3(2,v);
  ts3.update(ts1);
  ts3.update(ts2);

  Message a(1,1,ts1,MESSAGE,string("hey"),acknowledgements);
  Message b(2,2,ts2,MESSAGE,string("hey"),acknowledgements);
  Message c(3,3,ts3,MESSAGE,string("hey"),acknowledgements);

  assert(b < c);
  assert(a < c);
  assert(a < b);

  vector<Message> msgs;
  msgs.push_back(b);
  msgs.push_back(c);
  msgs.push_back(a);

  sort(msgs.begin(), msgs.end());

  for (vector<Message>::iterator it=msgs.begin(); it!=msgs.end(); ++it)
    std::cout << ' ' << (*it).getSenderId();
  std::cout << '\n';
}

int main() {
  //testSerialize();
  testMessage();
}

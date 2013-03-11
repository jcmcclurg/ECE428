#include <iostream>
#include <string>
#include <assert.h>

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

int main() {
  testSerialize();
}
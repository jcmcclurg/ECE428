#include "message.h"
#include "../timestamp/timestamp.h"
#include <assert.h>
#include <iostream>

Message::Message(
    int senderId, 
    int sequenceNumber, 
    Timestamp& timestamp,
    MessageType type, 
    string message, 
    map<int, int>& acknowledgements,
    set<int>& failedNodes) :
      senderId(senderId), 
      sequenceNumber(sequenceNumber), 
      type(type), 
      message(message), 
      acknowledgements(acknowledgements),
      failedNodes(failedNodes),
      needsDelete(false) {
  this->timestamp = new Timestamp(timestamp);
}

static int munchInteger(const string& buffer, int& offset) {
  int i = *(reinterpret_cast<const int*>(buffer.c_str() + offset));
  offset += sizeof(int);
  return i;
}


Message::Message(const string& encoded) : needsDelete(true) {
  int offset = 0;

  // Type header
  int typeSize = munchInteger(encoded, offset);
  assert(typeSize == 1);
  char type = encoded[offset++];
  switch (type) {
    case 'H':
      type = HEARTBEAT;
      break;
    case 'R':
      type = RETRANSREQUEST;
      break;
    case 'M':
      type = MESSAGE;
      int messageSize = munchInteger(encoded, offset);
      message = string(encoded.c_str() + offset, messageSize);
      offset += messageSize;
  }

  // Sequence number
  int senderIdSize = munchInteger(encoded, offset);
  cout << senderIdSize << "bubba"<< endl;
  assert(senderIdSize == 1);
  senderId = munchInteger(encoded, offset);
  map<int,int> ts;

  // Sequence number
  int sequenceSize = munchInteger(encoded, offset);
  assert(sequenceSize == 1);
  sequenceNumber = munchInteger(encoded, offset);

  // Acknowledgements
  int ackListBytes = munchInteger(encoded, offset);
  int pairs = ackListBytes / (2 * sizeof(int));
  for (int i = 0; i < pairs; ++i) {
    int id = munchInteger(encoded, offset);
    int sequenceNumber = munchInteger(encoded, offset);
    acknowledgements[id] = sequenceNumber;
  }

  // Timestamp
  int timestampBytes = munchInteger(encoded, offset);
  int count = timestampBytes / sizeof(int);
  for (int i = 0; i < count; ++i) {
    int id = munchInteger(encoded, offset);
    int count = munchInteger(encoded, offset);
    ts[id] = count;
  }
  timestamp = new Timestamp(senderId, ts);

  // Failed nodes
  int failedNodesBytes = munchInteger(encoded, offset);
  count = failedNodesBytes / sizeof(int);
  for (int i = 0; i < count; ++i) {
    int id = munchInteger(encoded, offset);
    failedNodes.insert(id);
  }
}

static void appendInteger(string& buffer, int val) {
  char* ca = reinterpret_cast<char*>(&val);
  buffer.append(ca, sizeof(int));
}

//! Message format shown below. Each block is prefixed with a four byte length message.
//! A bit overkill, but would be needed for any sort of platform independent representation.
//! [message type][message (optional)][sender id][sequence number][acknowledgements][timestamp][failed nodes]
string Message::getEncodedMessage(){
  string result;

  // Type header
  appendInteger(result, 1); // Block header!
  switch (type) {
    case HEARTBEAT:
      result += 'H';
      break;
    case RETRANSREQUEST:
      result += 'R';
      break;
    case MESSAGE:
      result += 'M';
      appendInteger(result, message.size()); // Block header!
      result += message;
  }

  // Sender ID
  appendInteger(result, 1); // Block header!
  appendInteger(result, senderId);

  // Sequence number
  appendInteger(result, 1); // Block header!
  appendInteger(result, sequenceNumber);

  // Acknowledgements

  // Two integers (process ID and sequence number) per external node.
  int ackListBytes = acknowledgements.size() * 2 * sizeof(int);
  appendInteger(result, ackListBytes); // Block header!
  for (map<int, int>::iterator it = acknowledgements.begin(); 
      it != acknowledgements.end(); 
      ++it) {

    appendInteger(result, it->first);
    appendInteger(result, it->second);
  }

  // Timestamp
  int timestampBytes = timestamp->getTimestampMap().size() * sizeof(int);
  appendInteger(result, timestampBytes);
  for (
      map<int, int>::iterator it = timestamp->getTimestampMap().begin(); 
      it != timestamp->getTimestampMap().end(); 
      ++it) {

    appendInteger(result, it->first);
    appendInteger(result, it->second);
  }

  // Failed nodes
  int failedNodesBytes = failedNodes.size() * sizeof(int);
  appendInteger(result, failedNodesBytes);
  for (
      set<int>::iterator it = failedNodes.begin(); 
      it != failedNodes.end(); 
      ++it) {

    appendInteger(result, *it);
  }

  return result;
}

bool operator<(const Message& a, const Message& b){
  CausalityRelation r = a.timestamp->compare(*(b.timestamp));
  if(r == BEFORE){
    return true;
  }
  else if(r == AFTER){
    return false;
  }
  else
  {
    return a.senderId < b.senderId;
  }
}

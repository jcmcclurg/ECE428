#include "message.h"
#include "../operators.h"
#include "../timestamp/timestamp.h"
#include <assert.h>
#include <string.h>
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

int Message::getEncodedMessageSize(){
   return
      sizeof(char)                    // type
    + sizeof(char)*(message.size()+1) // message
    + sizeof(int)*2                   // seqNum
    + sizeof(int)*2                   // sendId
    + sizeof(int)*(1 + 2*acknowledgements.size())
    + sizeof(int)*(1 + acknowledgements.size())
    + sizeof(int)*(1 + failedNodes.size()-1);
}

static int munchInteger(const char** buffer) {
  int i = *(reinterpret_cast<const int*>(*buffer));
  *buffer += sizeof(int);
  return i;
}


Message::Message(const char* encoded, int len) : needsDelete(true) {
  int offset = 0;

  // Type header
  int typeSize = munchInteger(&encoded);
  assert(typeSize == 1);
  char type = *encoded; encoded++;
  switch (type) {
    case 'H':
      type = HEARTBEAT;
      break;
    case 'R':
      type = RETRANSREQUEST;
      break;
    case 'M':
      type = MESSAGE;
      int messageSize = munchInteger(&encoded);
      message = string(encoded, messageSize);
      encoded += messageSize;
  }

  // Sender ID
  int senderIdSize = munchInteger(&encoded);
  assert(senderIdSize == 1);
  senderId = munchInteger(&encoded);
  map<int,int> ts;

  // Sequence number
  int sequenceSize = munchInteger(&encoded);
  assert(sequenceSize == 1);
  sequenceNumber = munchInteger(&encoded);

  // Acknowledgements
  int ackListBytes = munchInteger(&encoded);
  int pairs = ackListBytes / (2 * sizeof(int));
  for (int i = 0; i < pairs; ++i) {
    int id = munchInteger(&encoded);
    int sequenceNumber = munchInteger(&encoded);
    acknowledgements[id] = sequenceNumber;
  }

  // Timestamp
  int timestampBytes = munchInteger(&encoded);
  int count = timestampBytes / sizeof(int);
  for (int i = 0; i < count; ++i) {
    int id = munchInteger(&encoded);
    int count = munchInteger(&encoded);
    ts[id] = count;
  }
  timestamp = new Timestamp(senderId, ts);

  // Failed nodes
  int failedNodesBytes = munchInteger(&encoded);
  count = failedNodesBytes / sizeof(int);
  for (int i = 0; i < count; ++i) {
    int id = munchInteger(&encoded);
    failedNodes.insert(id);
  }
}

static void appendInteger(char** buffer, int val) {
  char* ca = reinterpret_cast<char*>(&val);
  memcpy(*buffer, ca, sizeof(int));
  *buffer += sizeof(int);
}

//! Message format shown below. Each block is prefixed with a four byte length message.
//! A bit overkill, but would be needed for any sort of platform independent representation.
//! [message type][message (optional)][sender id][sequence number][acknowledgements][timestamp][failed nodes]
int Message::getEncodedMessage(char* result){
  char* resultStart = result;

  // Type header (and message)
  appendInteger(&result, 1); // Block header!
  switch (type) {
    case HEARTBEAT:
      *result = 'H'; result++;
      break;
    case RETRANSREQUEST:
      *result = 'R'; result++;
      appendInteger(&result, message.size());
      memcpy(result, message.c_str(), message.size()+1);
      result += message.size()+1;
      break;
    case MESSAGE:
      *result = 'M'; result++;
      appendInteger(&result, message.size()+1);
      memcpy(result, message.c_str(), message.size()+1);
      result += message.size()+1;
  }

  // Sender ID
  appendInteger(&result, 1); // Block header!
  appendInteger(&result, senderId);

  // Sequence number
  appendInteger(&result, 1); // Block header!
  appendInteger(&result, sequenceNumber);

  // Acknowledgements

  // Two integers (process ID and sequence number) per external node.
  int ackListBytes = acknowledgements.size() * 2 * sizeof(int);
  appendInteger(&result, ackListBytes); // Block header!
  for (map<int, int>::iterator it = acknowledgements.begin(); 
      it != acknowledgements.end(); 
      ++it) {

    appendInteger(&result, it->first);
    appendInteger(&result, it->second);
  }

  // Timestamp
  int timestampBytes = timestamp->getTimestampMap().size() * sizeof(int);
  appendInteger(&result, timestampBytes);
  for (
      map<int, int>::iterator it = timestamp->getTimestampMap().begin(); 
      it != timestamp->getTimestampMap().end(); 
      ++it) {

    appendInteger(&result, it->first);
    appendInteger(&result, it->second);
  }

  // Failed nodes
  int failedNodesBytes = failedNodes.size() * sizeof(int);
  appendInteger(&result, failedNodesBytes);
  for (
      set<int>::iterator it = failedNodes.begin(); 
      it != failedNodes.end(); 
      ++it) {

    appendInteger(&result, *it);
  }

  return result - resultStart;
}

ostream& operator<<(ostream& strm, const Message& m){
  strm << "Message{"
       << m.senderId << ","
       << m.sequenceNumber << ","
       << *m.timestamp << ","
       << m.type << ","
       << m.message << ","
       << m.acknowledgements << ","
       << m.failedNodes
       << "}";
  return strm;
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

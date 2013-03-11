#include "message.h"
#include <assert.h>

Message::Message(
    int senderId, 
    int sequenceNumber, 
    Timestamp& timestamp,
    MessageType type, 
    string message, 
    vector<pair<int, int> > acknowledgements) :
      senderId(senderId), 
      sequenceNumber(sequenceNumber), 
      type(type), 
      message(message), 
      acknowledgements(acknowledgements) {

  this->timestamp = new Timestamp(timestamp);      
}

Message::~Message() {
  delete timestamp;
}

int munchInteger(const string& buffer, int& offset) {
  int i = *(reinterpret_cast<int*>(buffer.c_str()[offset]));
  offset += sizeof(int);
  return i;
}

Message::Message(const string& encoded) {
  int offset = 0;

  // Type header
  int typeSize = munchInteger(encoded, offset);
  assert(typeSize == 1);
  char type = encoded[offset];
  switch (type) {
    case 'H':
      type = HEARTBEAT;
      break;
    case 'R':
      type = RETRANSREQUEST;
      break;
    case MESSAGE:
      type = MESSAGE;
      int messageSize = munchInteger(encoded, offset);
      message = string(encoded.c_str() + offset, messageSize);
  }

  // Sequence number
  int senderIdSize = munchInteger(encoded, offset);
  assert(senderIdSize == 1);
  senderId = munchInteger(encoded, offset);
  timestamp = new Timestamp(senderId);

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
    acknowledgements.push_back(make_pair<int, int>(id, sequenceNumber));
  }

  // Timestamp
  int timestampBytes = munchInteger(encoded, offset);
  int values = timestampBytes / sizeof(int);
  for (int i = 0; i < values; ++i) {
    int id = munchInteger(encoded, offset);
    int count = munchInteger(encoded, offset);
    timestamp[id] = count;
  } 
}

void appendInteger(string& buffer, int val) {
  char* ca = reinterpret_cast<char*>(&val);
  buffer.append(ca, sizeof(int));
}

//! Message format shown below. Each block is prefixed with a four byte length message.
//! A bit overkill, but would be needed for any sort of platform independent representation.
//! [message type][message (optional)][sender id][sequence number][acknowledgements][timestamp]
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
      result += message.size();
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
  for (vector<pair<int, int> >::iterator it = acknowledgements.begin(); 
      it != acknowledgements.end(); 
      ++it) {

    appendInteger(result, it->first);
    appendInteger(result, it->second);
  }

  // Timestamp
  int timestampBytes = timestamp->getTimestamp().size() * sizeof(int);
  appendInteger(result, timestampBytes);
  for (
      map<int, int>::iterator it = timestamp->getTimestamp().begin(); 
      it != timestamp->getTimestamp().end(); 
      ++it) {

    appendInteger(result, it->first);
    appendInteger(result, it->second);
  }
}

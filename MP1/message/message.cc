#include "message.h"

Message::Message(const string& encoded){
  int offset = 1;
  while (offset < total_bytes) {
    if (combined_message[offset - 1] == MESSAGE_HEADER) {
      int bytes = strlen(combined_message + offset) + 1;
      messages.push_back(combined_message + offset);
      offset += bytes + 1;
    } else if (combined_message[offset - 1] == TIMESTAMP_HEADER) {
      timestamp = (int*) (combined_message + offset);
      return;
    } else {
      printf("Error! Poorly formatted message!\n");
      return;
    }
  }
}

Message::Message(GlobalState& globalState, MessageType type, string message)
    : globalState(globalState), type(type), message(message) {}


string Message::getMessage(){
  return message;
}

//! Message format shown below. Delimited as Pascal strings.
// [type header][message (optional)][sequence number][acks list][timestamp]
string Message::getEncodedMessage(){
  string result;

  // Type header
  result += 1;
  switch (type) {
    case HEARTBEAT:
      result += 'H';
      break;
    case RETRANSREQUEST:
      result += 'R';
      break;
    case MESSAGE:
      result += 'M';
      result += message.size();
      result += message;
  }

  // Sequence number
  result += 1;
  result += sequenceNumber;

  // Acks list
  result += 2 * sizeof(int) * globalState.externalStates.size();
  for (
      map<int, ExternalNodeState>::iterator it = globalState.externalStates.begin(); 
      it != globalState.externalStates.end(); 
      ++it){

    result += 
  }

  // Timestamp
  result += 
}

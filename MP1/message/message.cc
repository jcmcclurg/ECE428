#include "message.h"

Message::Message(string encodedMessage){
 //FIXME: Implement
}


string Message::getMessage(){
  return message;
}

string Message::getEncodedMessage(){
  //FIXME: Implement
  return NULL;
}

/**
* Decodes timestamp, message, and metadata from packets encoded with flatten.
* @param [in]  combined_message
* @param [in]  total_bytes
* @param [out] messages
* @param [out] timestamp
*/
void Message::unflatten(
    const char* combined_message,
    int total_bytes,
    vector<const char*>& messages,
    int*& timestamp) {

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
/**
* FIXME: Move to Message constructor
* Encodes timestamp, message, and metadata into serialized packet for transmission.
* @param [in]  messages
* @param [in]  timestamp
* @param [out] combined_message
* @param [out] total_bytes
*/
void Message::flatten(
    std::vector<const char*>& messages,
    int* timestamp,
    char*& combined_message,
    int& total_bytes) {

  int *message_bytes = (int*) calloc(messages.size(), sizeof(int)); 
  int timestamp_bytes = mcast_num_members * sizeof(int);

  total_bytes = 0;
  for (int i = 0; i < messages.size(); ++i) {
    int bytes = strlen(messages[i]) + 1;
    message_bytes[i] = bytes;
    total_bytes += bytes;
  }
    // The extra bytes are for the control headers. 
  total_bytes += messages.size() + 1 + timestamp_bytes;

  combined_message = (char*) calloc(total_bytes, 1);

  int offset = 0;
  for (int i = 0; i < messages.size(); ++i) {
    combined_message[offset] = MESSAGE_HEADER;
    offset++;
    memcpy(combined_message + offset, messages[i], message_bytes[i]);
    offset += message_bytes[i];
  }

    // Add timestamp
  combined_message[offset] = TIMESTAMP_HEADER;
  offset++;
  memcpy(combined_message + offset, timestamp, timestamp_bytes);

  free(message_bytes);
  }
}

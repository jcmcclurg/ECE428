#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MESSAGE_HEADER '\x1f'
#define TIMESTAMP_HEADER '\x1d'

#include "../delivery/delivery_ack.h"
#include "../timestamp/timestamp.h"
#include <set>
#include <string>
#include <vector>

using namespace std;

enum MessageType {HEARTBEAT, RETRANSREQUEST, MESSAGE};

class Message{
  private:
    MessageType type;
    int sequenceNumber;

    //! Process IDs and last delivered sequence numbers.
    DeliveryAcks* deliveryAcknowledgeList;
    set<int>* failedNodeList;

    //! ID of the original sender of the message
    int id;

    string message;
    Timestamp* timestamp;

  public:   
    Message(string encodedMessage);

    Message(
        MessageType type,
        int sequenceNumber,
        int id,
        DeliveryAcks* deliveryAcknowledgeList,
        vector<int>* failedNodeList,
        string message,
        Timestamp* timestamp);

    string getMessage();
    string getEncodedMessage();

    /**
     * Decodes timestamp, message, and metadata from packets encoded with flatten.
     * @param [in]  combined_message
     * @param [in]  total_bytes
     * @param [out] messages
     * @param [out] timestamp
     */
    void unflatten(
      const char* combined_message,
      int total_bytes,
      std::vector<const char*>& messages,
      int*& timestamp);

    /**
     * FIXME: Move to Message constructor
     * Encodes timestamp, message, and metadata into serialized packet for transmission.
     * @param [in]  messages
     * @param [in]  timestamp
     * @param [out] combined_message
     * @param [out] total_bytes
     */
    void flatten(
        std::vector<const char*>& messages,
        int* timestamp,
        char*& combined_message,
        int& total_bytes);
};

#endif

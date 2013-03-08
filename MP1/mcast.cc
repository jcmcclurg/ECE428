#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <vector>
#include <algorithm>
#include <ctime>
#include <sstream>

using namespace std;

#include "mp1.h"

/* Defines */

#define MESSAGE_HEADER '\x1f'
#define TIMESTAMP_HEADER '\x1d'

#define HEARTBEAT_DELAY 10
#define HEARTBEAT_MESSAGE "xoxo"

#define TIMEOUT_SECONDS 60

enum CausalityRelation {BEFORE,CONCURRENT,AFTER};
enum MessageType {HEARTBEAT,RETRANSREQUEST,MESSAGE};

class DeliveryAcks{
   private:
      int own_id;
      map<int,int> deliveryAcks;

   public:
      DeliveryAcks(int id){
         own_id = id;
         deliveryAcks[own_id] = 0;
      }

      void deleteMember(int id){
         deliveryAcks.erase(id);
      }

      void ack(int fromId , int sequenceNumber){
         deliveryAcks[fromId] = sequenceNumber;
      }
};

class Timestamp{
   private:
      int own_id;
      map<int,int> timestamp;

   public:
      Timestamp(int id){
         own_id = id;
         timestamp[own_id] = 0;
      }

      void deleteMember(int id){
         timestamp.erase(id);
      }

      void addMember(int id){
         timestamp[id] = 0;
      }

      /**
       * Increments own counter.
       */
      void step(){
         timestamp[own_id]++;
      }

      /**
       * Compares the vector timestamps to determine causality relation.
       */
      CausalityRelation compare(Timestamp* t){
         int numLess = 0;
         int numGreater = 0;
         int n;
         
         for (map<int,int>::iterator it=timestamp.begin(); it != timestamp.end(); ++it){
            if(it->first < t->timestamp[it->second]){
               numLess++;
               if(numGreater > 0){
                  return CONCURRENT;
               }
            }
            else if(it->first > t->timestamp[it->second]){
               numGreater++;
               if(numLess > 0){
                  return CONCURRENT;
               }
            }
         }

         if(numLess > 0){
            return BEFORE;
         }
         else if(numGreater > 0){
            return AFTER;
         }
         else{
            return CONCURRENT;
         }
      }

      /**
       * Updates the vector timestamp to include information from another timestamp.
       */
      void update(Timestamp* t){
         //FIXME: Implement
      }
};

class Message{
   private:
      MessageType type;
      int sequenceNumber;

      //! Process IDs and last delivered sequence numbers.
      DeliveryAcks* deliveryAcknowledgeList;
      vector<int>* failedNodeList;

      //! ID of the original sender of the message
      int id;

      string message;
      Timestamp* timestamp;

   public:   
      Message(string encodedMessage){
         //FIXME: Implement
      }

      Message(MessageType tp,
              int seqNum,
              int idd,
              DeliveryAcks* deliveryAckList,
              vector<int>* failedNdeList,
              Timestamp* ts,
              string message){
         type = tp;
         sequenceNumber = seqNum;
         failedNodeList = failedNdeList;
         id = idd;
         deliveryAcknowledgeList = deliveryAckList;
         timestamp = ts;

      }

      string getMessage(){
         return message;
      }

      string getEncodedMessage(){
         //FIXME: Implement
         return NULL;
      }
};

/* State variables */
class NodeState{
   private:
      //! The time we last heard from this node.
      time_t lastHeardFrom;

      //! Sequence number of the last message delivered from this node.
      int latestDeliveredSequenceNumber;

      //! List of all messages sent by this node.
      map<int, Message> undeliveredMessages;

   public:
      //! Vector timestamp, with included IDs for easy access.
      Timestamp* timestamp;

      NodeState(Timestamp* ts){
         timestamp = ts;
      }
      bool isTimedOut(){
         return time(NULL) - lastHeardFrom >= TIMEOUT_SECONDS;
      }

      void resetTimeout(){
         lastHeardFrom = time(NULL);
      }
};

//! Mapping from ID to node state
map<int, NodeState*> global_state;

//! The following pointer lists are used to update all the data structures when group membership changes.
vector<Timestamp*> timestamp_list;
vector<DeliveryAcks*> deliveryAck_list;

/**
 * Initializes the global state map.
 */
void state_init(void) {
	for (int i = 0; i < mcast_num_members; ++i) {
      Timestamp* t= new Timestamp(mcast_members[i]);
      timestamp_list.push_back(t);
		global_state[mcast_members[i]] = new NodeState(t);
		global_state[mcast_members[i]]->resetTimeout();
	}
   DeliveryAcks* d = new DeliveryAcks(my_id);
   deliveryAck_list.push_back(d);
}

/**
 * Increment the vector timestamp.
 */
void timestamp_increment() {
   global_state[my_id]->timestamp->step();
}

/**
 * Merges external timestamp with the existing timestamp. Assumes other_timestamp is same size and is ordered the same as local timestamp.
 * @param [in] other_timestamp
 */
void timestamp_merge(Timestamp* other_timestamp){
   global_state[my_id]->timestamp->update(other_timestamp);
}

/**
 * Updates global state to include a new multicast member.
 * @param [in] member
 */
void mcast_join(int member) {
	// Make certain this really is a new member. If not, don't do anything.
   // Update global_state, timestamp_list, and deliveryAck_list.
   // FIXME: Implement.
}

/**
 * Updates global state to exclude an existing multicast member.
 * @param [in] member
 */
void mcast_kick(int member) {
	// Make certain this really is a person in the group. If not, don't do anything.
   // Update global_state, timestamp_list, and deliveryAck_list.
   // FIXME: Implement.
}

/**
 * De-allocates the timestamp memory.
 */
void state_teardown() {
   // Do opposite of state_init()
   // FIXME: Implement.
}

/* General */

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

/**
 * FIXME: Move to Message constructor
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

/* Failure detection */
bool* failed_processes;
pthread_t heartbeat_thread;

void *heartbeat_thread_main(void *arg) {
    timespec req;
    req.tv_sec = 0;
    req.tv_nsec = HEARTBEAT_DELAY * 1000000L;
    while(1) {
        nanosleep(&req, NULL);
    }
}

void heartbeat_init(void) {
    if (pthread_create(&heartbeat_thread, NULL, &heartbeat_thread_main, NULL) != 0) {
        fprintf(stderr, "Error creating heartbeat thread!\n");
        exit(1);
    }
}

void heartbeat_destroy(void) {

}

void multicast_init(void) {
    unicast_init();
    state_init();
}

/* Basic multicast implementation */
void multicast(const char *message) {
    timestamp_increment();
    
    char* combined_message = NULL;
    int total_bytes;
    int* timestamp;

    std::vector<const char*> messages;
    messages.push_back("abc");
    messages.push_back("omg");

    flatten(messages, timestamp, combined_message, total_bytes);

    pthread_mutex_lock(&member_lock);
    for (int i = 0; i < mcast_num_members; i++) {
        usend(mcast_members[i], combined_message, total_bytes);
    }
    pthread_mutex_unlock(&member_lock);

    free(combined_message);
}

void receive(int source, const char *message, int len) {
    //assert(message[len-1] == 0);

    std::vector<const char*> messages;
    int* other_timestamp;    
    unflatten(message, len, messages, other_timestamp);

    deliver(source, message);
}


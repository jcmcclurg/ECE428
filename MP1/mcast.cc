#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <iostream>
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

  virtual ostream& dump(ostream& strm) const{
    strm << "DeliveryAcks{\n";
    for(map<int,int>::const_iterator it=deliveryAcks.begin(); it != deliveryAcks.end(); ++it){
      if(it->first != own_id){
        strm << "  "<< it->first << "  = " << it->second << "\n";
      }
      else{
        strm << " ["<< it->first << "] = " << it->second << "\n";
      }
    }
    strm << "}\n";
    return strm;
  }
  
  /**
   * Delete the member with the specified id from the list.
   * @param id
   */
  void deleteMember(int id){
    deliveryAcks.erase(id);
  }
  
  /**
   * Mark the specified sequence number as being delivered.
   */
  void ack(int fromId , int sequenceNumber){
    deliveryAcks[fromId] = sequenceNumber;
  }

  /**
   * Updates the acknowledgement list to include information from another list.
   * Only allows increments, and only updates if id of other list matches its own.
   * @param [in] otherList
   */
  void update(DeliveryAcks* t){
    if(own_id == t->own_id){
      for (map<int,int>::iterator it=deliveryAcks.begin(); it != deliveryAcks.end(); ++it){
        if(deliveryAcks[it->first] < t->deliveryAcks[it->first]){
          deliveryAcks[it->first] = t->deliveryAcks[it->first];
        }
      }
    }
  }
};

ostream& operator<<(ostream &strm, const DeliveryAcks* a){
  return a->dump(strm);
}

ostream& operator<<(ostream &strm, const DeliveryAcks &a){
  return a.dump(strm);
}

class Timestamp{
private:
  friend ostream& operator<<(ostream &strm, const Timestamp &a);
  int own_id;
  map<int,int> timestamp;
  
public:
  Timestamp(int id){
    own_id = id;
    timestamp[own_id] = 0;
  }

  Timestamp(int id, map<int,int> timestampMap){
    own_id = id;
    timestamp = timestampMap;
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
      if(it->second < t->timestamp[it->first]){
        numLess++;
        if(numGreater > 0){
          return CONCURRENT;
        }
      }
      else if(it->second > t->timestamp[it->first]){
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

  virtual ostream& dump(ostream& strm) const{
    strm << "Timestamp{\n";
    for(map<int,int>::const_iterator it=timestamp.begin(); it != timestamp.end(); ++it){
      if(it->first != own_id){
        strm << "  "<< it->first << "  = " << it->second << "\n";
      }
      else{
        strm << " ["<< it->first << "] = " << it->second << "\n";
      }
    }
    strm << "}\n";
    return strm;
  }
  
  /**
   * Updates the vector timestamp to include information from another timestamp.
   */
  void update(Timestamp* t){
    for (map<int,int>::iterator it=timestamp.begin(); it != timestamp.end(); ++it){
      if(it->first != own_id && it->second < t->timestamp[it->first]){
        timestamp[it->first] = t->timestamp[it->first];
      }
    }
  }
};

ostream& operator<<(ostream &strm, const Timestamp* a){
  return a->dump(strm);
}

ostream& operator<<(ostream &strm, const Timestamp &a){
  return a.dump(strm);
}

class Message{
private:
  MessageType type;
  
  //! Process IDs and last delivered sequence numbers.
  DeliveryAcks* deliveryAcknowledgeList;
  vector<int>* failedNodeList;
  
  //! ID of the original sender of the message
  int id;
  
  string message;
  Timestamp* timestamp;
  
public:   
  int sequence_number;
  Message(string encodedMessage){
    //FIXME: Implement
  }
  
  Message(MessageType tp,
      int seqNum,
      int idd,
      DeliveryAcks* deliveryAckList,
      vector<int>* failedNdeList,
      Timestamp* ts,
      string msg){
    type = tp;
    sequence_number = seqNum;
    failedNodeList = failedNdeList;
    id = idd;
    deliveryAcknowledgeList = deliveryAckList;
    timestamp = ts;
    message = msg;
  }

  virtual ostream& dump(ostream& strm) const{
    strm << "Message{"<< sequence_number << " from "<< id << "}";
  }

  ~Message(){
    // We don't delete timestamp and deliveryAcknowledgeList because those
    // pointers are managed by the global lists.
  }
  
  string getMessage(){
    return message;
  }
  
  string getEncodedMessage(){
    //FIXME: Implement
    return NULL;
  }
};

ostream& operator<<(ostream &strm, const Message* a){
  return a->dump(strm);
}

ostream& operator<<(ostream &strm, const Message &a){
  return a.dump(strm);
}

/* State variables */
class NodeState{
private:
  int id;

  //! The time we last heard from this node.
  time_t lastHeardFrom;
  
  //! Sequence number of the last message delivered from this node.
  int latestDeliveredSequenceNumber;
  
public:
  //! List of all messages sent by this node, and not yet known to be delivered to all. Indexed by sequence number.
  map<int, Message*> sentMessages;

  DeliveryAcks* deliveryAckList;

  NodeState(int idd, DeliveryAcks* d){
    id = idd;
    lastHeardFrom = 0;
    deliveryAckList = d;
  }

  ~NodeState(){
    for (map<int,Message*>::iterator it=sentMessages.begin(); it != sentMessages.end(); ++it){
      delete it->second;
    }

    // Don't delete deliveryAckList because this pointer is managed by the global list.
  }

  virtual ostream& dump(ostream& strm) const{
    strm << "NodeState{" << id << "}";
  }

  bool isTimedOut(){
    return time(NULL) - lastHeardFrom >= TIMEOUT_SECONDS;
  }
  
  void resetTimeout(){
    lastHeardFrom = time(NULL);
  }
};

ostream& operator<<(ostream &strm, const NodeState* a){
  return a->dump(strm);
}

ostream& operator<<(ostream &strm, const NodeState &a){
  return a.dump(strm);
}

//! The global state information
Timestamp* global_timestamp;
map<int, NodeState*> global_state;
vector<int> failed_nodes;
int sequence_number;

//! The following pointer lists are used to update all the relevent data structures when group membership changes.
vector<Timestamp*> timestamp_list;
vector<DeliveryAcks*> deliveryAck_list;

/**
 * Initializes the global state map.
 */
void state_init(void) {
  global_timestamp = new Timestamp(my_id);
  timestamp_list.push_back(global_timestamp);
}

/**
 * Increment the vector timestamp.
 */
void timestamp_increment() {
  global_timestamp->step();
}

/**
 * Merges external timestamp with the existing timestamp. Assumes other_timestamp is same size and is ordered the same as local timestamp.
 * @param [in] other_timestamp
 */
void timestamp_merge(Timestamp* other_timestamp){
  global_timestamp->update(other_timestamp);
}

/**
 * Updates global state to include a new multicast member.
 * @param [in] member
 */
void mcast_join(int member) {
  //! Make certain this really is a new member.
  if(find(failed_nodes.begin(),failed_nodes.end(), member) != failed_nodes.end() &&
     global_state.find(member) != global_state.end()){
    cerr << "ERROR: Member " << member << " already joined.\n";
    exit(1);
  }

  //! Update global_state, timestamp_list, and deliveryAck_list.
  DeliveryAcks* d = new DeliveryAcks(member);
  deliveryAck_list.push_back(d);
  global_state[member] = new NodeState(member,d);

  for (vector<Timestamp*>::iterator it=timestamp_list.begin(); it != timestamp_list.end(); ++it){
    (*it)->addMember(member);
  }
}

/**
 * Updates global state to exclude an existing multicast member.
 * @param [in] member
 */
void mcast_kick(int member) {
  //! Make certain this really is a person in the group. If not, don't do anything.
  if(global_state.find(member) == global_state.end()){
    cerr << "ERROR: Member " << member << " already kicked.\n";
    exit(1);
  }

  //! Update global_state, timestamp_list, and deliveryAck_list.
  delete global_state[member];
  global_state.erase(member);

  for (vector<Timestamp*>::iterator it=timestamp_list.begin(); it != timestamp_list.end(); ++it){
    (*it)->deleteMember(member);
  }

  for (vector<DeliveryAcks*>::iterator it=deliveryAck_list.begin(); it != deliveryAck_list.end(); ++it){
    (*it)->deleteMember(member);
  }

  failed_nodes.push_back(member);
}

/**
 * De-allocates the timestamp memory. Does the opposite of state_init().
 */
void state_teardown() {
  for (map<int,NodeState*>::iterator it=global_state.begin(); it != global_state.end(); ++it){
    delete global_state[it->first];
  }

  for (vector<Timestamp*>::iterator it=timestamp_list.begin(); it != timestamp_list.end(); ++it){
    delete *it;
  }

  for (vector<DeliveryAcks*>::iterator it=deliveryAck_list.begin(); it != deliveryAck_list.end(); ++it){
    delete *it;
  }
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

void add_to_sent_list(Message* m){
  global_state[my_id]->sentMessages[m->sequence_number] = m;
}

void multicast_init(void) {
  unicast_init();
  state_init();
}

void multicast_deliver(int id, Message* m){
  deliver(my_id, (char*) m->getMessage().c_str());
}

/* Basic multicast implementation */
void multicast(const char *message) {
  //! Increment the timestamp before you do anything else.
  timestamp_increment();

  //! Wrap the message in a Message object.
  Message* m = new Message(MESSAGE,
    sequence_number,
    my_id,
    global_state[my_id]->deliveryAckList,
    &failed_nodes,
    global_timestamp,
    string(message));

  //! Deliver to yourself first, so that if you fail before sending it to everyone, the message can still get re-transmitted by someone else without violating the R-multicast properties.
  multicast_deliver(my_id, m);

  //! Add to the sent messages list so you can grab this message again in case a retransmission is needed.
  add_to_sent_list(m);

  pthread_mutex_lock(&member_lock);
  for (int i = 0; i < mcast_num_members; i++) {
      usend(mcast_members[i], message, strlen(message)+1);
  }
  pthread_mutex_unlock(&member_lock);

  //! Increment your own sent message sequence number.
  sequence_number++;
}

void receive(int source, const char *message, int len) {
  assert(message[len-1] == 0);

  if(source != my_id){
    deliver(source, message);
  }
}

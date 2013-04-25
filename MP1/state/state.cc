#include "state.h"
#include <algorithm>
#include <queue>
#include <set>
#include <iostream>

using namespace std;

Node::Node(int id, int* memberIds, int memberCount) : id(id), sequenceNumber(0), timestamp(id, memberIds, memberCount) {}

void Node::updateFailedNodes(set<int>& otherFailedNodes) {
  failedNodes.insert(otherFailedNodes.begin(), otherFailedNodes.end());
}

void ExternalNode::updateDeliveryAckList(map<int,int>& list){
  #ifdef DEBUG
  cout << "Updating external state[" << id << "] deliveryAckList = {";
  #endif
  for (
      map<int, int>::iterator it = list.begin();
      it != list.end();
      it++) {

    deliveryAckList[it->first] = max(deliveryAckList[it->first], it->second);
    #ifdef DEBUG
    cout << it->first << "=" << it->second << " ";
    #endif
  }
  #ifdef DEBUG
  cout << "}" << endl; 
  #endif
}

GlobalState::GlobalState(int ownId, int* memberIds, int memberCount) : node(ownId, memberIds, memberCount) {
  #ifdef DEBUG
    cout << "Global state of " << ownId << " instantiated with " << memberCount << " members: ";
  #endif
  for (int i = 0; i < memberCount; ++i) {
    if (memberIds[i] != ownId) {
      #ifdef DEBUG
        cout << memberIds[i] << " ";
      #endif
      externalNodes[memberIds[i]] = new ExternalNode(memberIds[i]);
    }
  } 
  #ifdef DEBUG
    cout << endl;
  #endif
}

GlobalState::~GlobalState() {
  for (set<Message*>::iterator it = messageStore.begin(); it != messageStore.end(); ++it) {
    delete *it;
  }

  for (
      map<int, ExternalNode*>::iterator it = externalNodes.begin();
      it != externalNodes.end();
      ++it) {

    delete it->second;
  }
}

Message* GlobalState::getMessage(int fromId, int sequenceNumber) {
  for (set<Message*>::iterator it = messageStore.begin(); it != messageStore.end(); ++it) {
    Message* m = *it;
    if(m->getSenderId() == fromId && m->getSequenceNumber() == sequenceNumber) {
      return m;
    }
  }
  return NULL;
}

void GlobalState::storeMessage(Message* m) {
  messageStore.insert(m);
  #ifdef DEBUG
    cout << "Storing message from" << m->getSenderId() << ": " << *m << endl;
  #endif
}
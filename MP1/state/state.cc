#include "state.h"
#include <algorithm>
#include <queue>
#include <set>
#include <iostream>

using namespace std;

NodeState::NodeState(int id, int* memberIds, int memberCount) : id(id), sequenceNumber(0), timestamp(id, memberIds, memberCount) {}
NodeState::~NodeState(){
  for (set<Message*>::iterator it=messageStore.begin(); it!=messageStore.end(); ++it){
    delete *it;
  }
}

Message* NodeState::getMessage(int sequenceNumber) {
  for (set<Message*>::iterator it = messageStore.begin(); it != messageStore.end(); ++it){
    if((*it)->getSequenceNumber() == sequenceNumber) {
      return *it;
    }
  }
  return NULL;
}

void NodeState::storeMessage(Message* m) {
  messageStore.insert(m);
  #ifdef DEBUG
    cout << "Storing message from "<< id << ": " << *m << endl;
  #endif
}

void NodeState::updateFailedNodes(set<int>& otherFailedNodes) {
  failedNodes.insert(otherFailedNodes.begin(), otherFailedNodes.end());
}

void ExternalNodeState::storeMessage(Message* m) {
  messageStore.insert(m);
  #ifdef DEBUG
    cout << "Storing message from "<< id << ": " << *m << endl;
  #endif
}

ExternalNodeState::~ExternalNodeState(){
  for (set<Message*>::iterator it=messageStore.begin(); it!=messageStore.end(); ++it){
    delete *it;
  }
}

void ExternalNodeState::updateDeliveryAckList(map<int,int>& list){
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

Message* ExternalNodeState::getMessage(int sequenceNumber) {
  for (set<Message*>::iterator it = messageStore.begin(); it != messageStore.end(); ++it){
    if((*it)->getSequenceNumber() == sequenceNumber) {
      return *it;
    }
  }
  return NULL;
}

GlobalState::GlobalState(int ownId, int* memberIds, int memberCount) : state(ownId, memberIds, memberCount) {
  #ifdef DEBUG
    cout << "Global state of " << ownId << " instantiated with " << memberCount << " members: ";
  #endif
  for (int i = 0; i < memberCount; ++i) {
    if (memberIds[i] != ownId) {
      #ifdef DEBUG
        cout << memberIds[i] << " ";
      #endif
      externalStates[memberIds[i]] = new ExternalNodeState(memberIds[i]);
    }
  } 
  #ifdef DEBUG
    cout << endl;
  #endif
}

GlobalState::~GlobalState() {
  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {

    delete it->second;
  }
}

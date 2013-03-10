#include "timestamp.h"

Timestamp::Timestamp(int own_id) : own_id(own_id){
  timestamp[own_id] = 0;
}

void Timestamp::deleteMember(int id){
  timestamp.erase(id);
}

void Timestamp::addMember(int id){
  timestamp[id] = 0;
}

void Timestamp::step(){
  timestamp[own_id]++;
}

CausalityRelation Timestamp::compare(Timestamp& t) const {
  int numLess = 0;
  int numGreater = 0;
  int n;

  for (map<int, int>::const_iterator it = timestamp.begin(); it != timestamp.end(); ++it) {
    if (it->first < t.timestamp[it->second]){ 
      numLess++;
      if (numGreater > 0) {
        return CONCURRENT;
      }
    }
    else if (it->first > t.timestamp[it->second]) {
      numGreater++;
      if (numLess > 0) {
        return CONCURRENT;
      }
    }
  }

  if (numLess > 0) { 
    return BEFORE;
  } else if (numGreater > 0) {
    return AFTER;
  } else {
    return CONCURRENT;
  }
}

void Timestamp::update(const Timestamp& t){

}
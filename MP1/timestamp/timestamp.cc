#include "timestamp.h"

Timestamp::Timestamp(int ownId) : ownId(ownId){
  timestamp[ownId] = 0;
}

void Timestamp::step(){
  timestamp[ownId]++;
}

CausalityRelation Timestamp::compare(Timestamp& t) const {
  int numLess = 0;
  int numGreater = 0;
  
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

void Timestamp::update(Timestamp& t){
  for (map<int, int>::iterator it=timestamp.begin(); it != timestamp.end(); ++it){
    if(it->first != ownId && it->second < t.timestamp[it->first]){
      timestamp[it->first] = t.timestamp[it->first];
    }
  }
}
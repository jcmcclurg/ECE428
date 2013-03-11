#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include <map>
#include <string>
#include <vector>

using namespace std;

enum CausalityRelation {BEFORE, CONCURRENT, AFTER};

class Timestamp {
  private:
    int ownId;
    map<int, int> timestamp;

  public:
    Timestamp(int id, int* memberIds, int memberCount);
    Timestamp(int id, map<int,int> timestampMap);
    Timestamp(int id, vector<int> ids);

    int getOwnId() const { return ownId; }
    map<int, int>& getTimestampMap() { return timestamp; } 

    /**
    * Increments own counter.
    */
    void step();

    /**
    * Compares the vector timestamps to determine causality relation.
    */
    CausalityRelation compare(Timestamp& t) const;

    /**
    * Updates the vector timestamp to include information from another timestamp.
    */
    void update(Timestamp& t);
};

#endif

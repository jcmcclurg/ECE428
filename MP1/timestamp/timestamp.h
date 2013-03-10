#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include <map>

using namespace std;

enum CausalityRelation {BEFORE, CONCURRENT, AFTER};

class Timestamp {
  private:
    int own_id;
    map<int, int> timestamp;

  public:
    Timestamp(int own_id);

    void deleteMember(int id);
    void addMember(int id);

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
    void update(const Timestamp& t);
};

#endif
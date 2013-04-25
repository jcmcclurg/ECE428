#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include <map>
#include <string>
#include <set>

using namespace std;

//! An enum representing a causality relation between two Timestamps.
enum CausalityRelation {BEFORE, CONCURRENT, AFTER};

//! A vector timestamp with convenient update, step, and compare methods.
class Timestamp {
  friend ostream& operator<<(ostream& strm, const Timestamp& m);
  private:
    //! The ID of the process that this timestamp belongs to.
    int ownId;

    //! A map from process ID to vector clock value.
    map<int, int> timestamp;

    //! A map from process ID to vector clock delta (since the last update call).
    map<int, int> diff;

  public:
    //! Construct a Timestamp from an ID and an array of additional group member IDs.
    //! All values are initially zero.
    Timestamp(int id, int* memberIds, int memberCount);

    //! Construct a Timestamp from an ID and an existing map.
    Timestamp(int id, map<int,int>& timestampMap);

    //! Construct a Timestamp from an ID and a set of IDs.
    //! All values are initially zero.
    Timestamp(int id, set<int>& ids);

    int getOwnId() const { return ownId; }
    map<int, int>& getTimestampMap() { return timestamp; } 

    //! Increments own counter.
    void step();

    //! Compares the vector timestamps to determine causality relation.
    //!
    //! @param t The timestamp to compare. 
    //! @return The causality relation between this timestamp and the compared timestamp.
    CausalityRelation compare(Timestamp& t) const;

    //! Updates the vector timestamp to include information from another timestamp.
    //! 
    //! @param t The timestamp to update from.
    //! @return A map of deltas between this timestamp and the external timestamp.
    map<int,int>& update(Timestamp& t);
};

#endif

#ifndef REPLICA_HANLDER_H
#define REPLICA_HANLDER_H

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include "Replica.h"

#include "model/statemachine.h"
#include "replicas.h"

namespace mp2 {

// Replica interface
class Replica : public ReplicaIf {
public:
	// My replica number
	const unsigned int id;
	Replica(int myid, StateMachineFactory & factory, boost::shared_ptr<Replicas> replicas); 
		// Do not change signature

	// RPC interface. This has to match the definitions in the gen-cpp/Replicas.h
	// as generated by thrift from the replica.thrift file
	virtual void create(const std::string& name, const std::string& initialState);
	virtual void apply(std::string& _return, const std::string& name, const std::string& operation);
	virtual void getState(std::string& _return, int16_t client, const std::string& name);
	virtual void remove(const std::string& name);

	int16_t prepareGetState(int16_t client, const std::string& name);
	int16_t getLeader(void);
	int16_t getQueueLen(void);
	int16_t getBwUtilization(void);
	int16_t getMemUtilization(void);
 	int16_t startLeaderElection(void);
	bool stateExists(const std::string& name);
	virtual void notifyFinishedReading(int16_t rmid, int16_t client, const std::string &name);

  	virtual void prepare(Promise& _return, const int32_t n);
  	virtual bool accept(const int32_t n, const int32_t value);
  	virtual void inform(const int32_t value);

	virtual void exit(void);

private:
	StateMachineFactory & factory; // used for creating new state machines
	boost::shared_ptr<Replicas> replicas; // used for communicating with other replicas

	typedef boost::unordered_map<std::string, boost::shared_ptr<StateMachine> > MachineMap;
 	MachineMap machines; // a collection of state machines indexed by name

 	// check to see if replica exists and throw a error otherwise
 	void checkExists(const std::string & name) const throw (ReplicaError);
 	// add any private methods and variables you need below. 

 	int leader;
	int queueLen;
	int bwUtilization;
	int memUtilization;

	std::map< std::string, std::vector< std::pair<char,std::string> > > requestQueue;

	// paxos
	int proposalNumber;
	int highestProposalNumber;
	int acceptedProposalNumber;
	int acceptedProposalValue;
	bool electionInProgress;

	void createReplicas(const std::string& name, const std::string& val);
	void reshuffleReplicas(const std::string& name, const std::string& val);
};

} // namespace mp2 

#endif

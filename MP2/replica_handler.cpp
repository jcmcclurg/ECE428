#include "settings.h"
#include "replica_handler.h"

using namespace mp2;
using namespace std;
using boost::shared_ptr;

#include <cstdlib>
#include <iostream>

Replica::Replica(int myid, StateMachineFactory & factory, shared_ptr<Replicas> replicas) : leader(-1), factory(factory), id(myid), replicas(replicas) {
	// any initialization you need goes here
	DEBUG( "Initialized RM " << myid );
}

int Replica::startLeaderElection(void) {
	// FIXME: Do actual leader election.
	DEBUG( "RM " << id << " started leader election...Returned 0" );
	return 0;
}

void Replica::checkExists(const string &name) const throw (ReplicaError) {
	if (machines.find(name) == machines.end()) {
		ReplicaError error;
		error.type = ErrorType::NOT_FOUND;
		error.name = name;
		error.message = string("Cannot find machine ") + name;
		throw error;
	}	
}

int16_t Replica::whoLeads(void){
	return leader;
}

bool Replica::stateExists(const std::string& name){
	if(leader != id){
		ReplicaError error;
		error.type = ErrorType::NOT_LEADER;
		error.name = name;
		error.message = string("Don't know whether state exists") + name;
		throw error;
	}
	return true;
}

int16_t Replica::whoHas(const std::string& name){
	if(leader != id){
		ReplicaError error;
		error.type = ErrorType::NOT_LEADER;
		error.name = name;
		error.message = string("Don't know who has that state") + name;
		throw error;
	}
	return 0;
}

void Replica::create(const string & name, const string & initialState) {
	if (machines.find(name) != machines.end()) {
		ReplicaError error;
		error.type = ErrorType::ALREADY_EXISTS;
		error.name = name;
		error.message = string("Machine ") + name + (" already exists");
		throw error;
	}
	if(leader == -1){
		leader = startLeaderElection();
	}
	DEBUG("RM " << id << " making a copy of " << name << ":" << initialState);
 	machines.insert(make_pair(name, factory.make(initialState)));
}

void Replica::apply(string & result, const string & name, const string& operation) {
	if(name == "" && operation == "leader?" && leader == (int)id){
		// Do not throw an exception, indicating that you are the leader.
	}
	else{
		if(leader == -1){
			leader = startLeaderElection();
		}
		checkExists(name);
		result = machines[name]->apply(operation);
	}
}

void Replica::getState(string& result, const string &name) {
	checkExists(name);
	if(leader == -1){
		leader = startLeaderElection();
	}

	result = machines[name]->getState();
}

void Replica::remove(const string &name) {
	checkExists(name);
	if(leader == -1){
		leader = startLeaderElection();
	}

	machines.erase(name);
}



/* DO NOT CHANGE THIS */
void Replica::exit(void) {
	clog << "Replica " << id << " exiting" << endl;
	::std::exit(0);	// no return
}


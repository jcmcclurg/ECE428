#include "settings.h"
#include "replica_handler.h"

using namespace mp2;
using namespace std;
using boost::shared_ptr;

#include <cstdlib>
#include <iostream>
#include "settings.h"

Replica::Replica(int myid, StateMachineFactory & factory, shared_ptr<Replicas> replicas) : leader(-1), factory(factory), id(myid), replicas(replicas), electionInProgress(false), queueLen(0), memUtilization(0), bwUtilization(0) {
	// any initialization you need goes here
	DEBUG( "Initialized RM " << myid );
}

int16_t Replica::startLeaderElection(void) {
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

void Replica::reshuffleReplicas(const std::string& name, const std::string& val){
	bool done;
	bool found = false;
	do{
		done = true;
		for(int i=0; i< (*replicas).numReplicas(); i++){
			if((*replicas)[i].stateExists(name) && (*replicas)[i].getMemUtilization() > 1){
				found = true;
				for(int j=0; j< (*replicas).numReplicas(); j++){
					if(!(*replicas)[j].stateExists(name) && (*replicas)[j].getMemUtilization() < (*replicas)[i].getMemUtilization()-1){
						//std::string ret;
						//(*replicas)[i].getState(ret,name);
						(*replicas)[j].create(name,val);
						(*replicas)[i].remove(name);
						done = false;
						break;
					}
				}
			}
		}
		if(!found){
			ReplicaError error;
			error.type = ErrorType::NOT_FOUND;
			error.name = name;
			error.message = string("Cannot find machine ") + name;
			throw error;
		}
	} while(!done);
}

void Replica::createReplicas(const std::string& name, const std::string& val){
	int count;
	do{
		int bm2 = INT_MAX;
		int bmi2;
		count = 0;
		// Find optimal locations to store and copy from
		for(int i=0; i< (*replicas).numReplicas(); i++){
			int b = (*replicas)[i].getBwUtilization();
			if((*replicas)[i].stateExists(name)){
				count++;
			}
			else{
				if((*replicas)[i].getMemUtilization() == 0){
					bm2 = i;
					break;
				}
				if(b < bm2){ bmi2 = i; bm2 = b; }
			}
		}
		if(count == 0){
			ReplicaError error;
			error.type = ErrorType::NOT_FOUND;
			error.name = name;
			error.message = string("Cannot find machine ") + name;
			throw error;
		}
		
		if(count < MIN_REPLICAS){
			//(*replicas)[bm].getState(ret,name);
			(*replicas)[bm2].create(name,val);
			count++;
		}
	}while(count < MIN_REPLICAS);
}
int16_t Replica::getLeader(void){
	if(leader == -1){
		leader = startLeaderElection();
	}
	return leader;
}
int16_t Replica::getQueueLen(void){
	return queueLen;
}
int16_t Replica::getBwUtilization(void){
	return bwUtilization;
}
int16_t Replica::getMemUtilization(void){
	return memUtilization;
}

int16_t Replica::prepareGetState(const std::string& name){
	if (id != leader){
		ReplicaError error;
		error.type = ErrorType::NOT_LEADER;
		error.name = name;
		error.message = string("Only the leader can prepare a get.");
		throw error;
	}

	int bm = INT_MAX;
	int bmi;
	bool success = false;

	for(int i=0; i< (*replicas).numReplicas(); i++){
		if((*replicas)[i].stateExists(name)){
			success = true;
			int b = (*replicas)[i].getBwUtilization();
			if(b < bm){ bmi = i; bm = b; }
		}
	}
	if(!success){
		ReplicaError error;
		error.type = ErrorType::NOT_FOUND;
		error.name = name;
		error.message = string("Cannot find machine ") + name;
		throw error;
	}
	return bmi;
}

bool Replica::stateExists(const std::string& name){
	try{
		checkExists(name);
		return true;
	}
	catch(ReplicaError){
		return false;
	}
}

void Replica::create(const string & name, const string & initialState) {
	if (machines.find(name) != machines.end()) {
		ReplicaError error;
		error.type = ErrorType::ALREADY_EXISTS;
		error.name = name;
		error.message = string("Machine ") + name + (" already exists");
		throw error;
	}
	queueLen++;
	memUtilization++;
 	machines.insert(make_pair(name, factory.make(initialState)));
	queueLen--;

//	if(id == leader){
//		createReplicas(name, initialState);
//		reshuffleReplicas(name, initialState);
//	}
}

void Replica::apply(string & result, const string & name, const string& operation) {
	checkExists(name);
	queueLen++;
	bwUtilization++;
	result = machines[name]->apply(operation);
	queueLen--;
}

void Replica::getState(string& result, const string &name) {
	checkExists(name);
	queueLen++;
	bwUtilization++;
	result = machines[name]->getState();
	queueLen--;
}

void Replica::remove(const string &name) {
	checkExists(name);
	queueLen++;
	memUtilization--;
	machines.erase(name);
	queueLen--;
}

/* DO NOT CHANGE THIS */
void Replica::exit(void) {
	clog << "Replica " << id << " exiting" << endl;
	::std::exit(0);	// no return
}

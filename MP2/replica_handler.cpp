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

std::vector<int> Replica::findReplicaManagers(const string& name){
	std::vector<int> v;
	for(int i=0; i< (*replicas).numReplicas(); i++){
		if((*replicas)[i].stateExists(name)){
			v.push_back(i);
		}
	}
	return v;
}

void Replica::reshuffleReplicas(const std::string& name){
	bool done;
	do{
		done = true;
		for(int i=0; i< (*replicas).numReplicas(); i++){
			if((*replicas)[i].stateExists(name) && (*replicas)[i].getMemUtilization() > 1){
				for(int j=0; j< (*replicas).numReplicas(); j++){
					if(!(*replicas)[j].stateExists(name) && (*replicas)[j].getMemUtilization() < (*replicas)[i].getMemUtilization()-1){
						std::string ret;
						(*replicas)[i].getState(ret,name);
						(*replicas)[j].create(name,ret);
						(*replicas)[i].remove(name);
						done = false;
						break;
					}
				}
			}
		}
	} while(!done);
}

void Replica::createReplicas(const std::string& name){
	int count;
	do{
		int bm = INT_MAX;
		int bmi;
/*		int qm = INT_MAX;
		int qmi;
		int mm = INT_MAX;
		int mmi;
*/
		int bm2 = INT_MAX;
		int bmi2;
/*		int qm2 = INT_MAX;
		int qmi2;
		int mm2 = INT_MAX;
		int mmi2;
*/
		count = 0;
		// Find optimal locations to store and copy from
		for(int i=0; i< (*replicas).numReplicas(); i++){
			int b = (*replicas)[i].getBwUtilization();
//			int q = (*replicas)[i].getQueueLen();
//			int m = (*replicas)[i].getMemUtilization();
			if((*replicas)[i].stateExists(name)){
				count++;
				if(b < bm){ bmi = i; bm = b; }
//				if(q < qm){ qmi = i; qm = q; }
//				if(m < mm){ mmi = i; mm = m; }
			}
			else{
				if(b < bm2){ bmi2 = i; bm2 = b; }
//				if(q < qm2){ qmi2 = i; qm2 = q; }
//				if(m < mm2){ mmi2 = i; mm2 = m; }
			}
		}
		if(count < MIN_REPLICAS){
			// For now, we only do a very simple cost function, but the other
			// variables are there for the future if need be.
			std::string ret;
			(*replicas)[bm].getState(ret,name);
			(*replicas)[bm2].create(name,ret);
			count++;
		}
	}while(count < MIN_REPLICAS);
}
int16_t Replica::getLeader(void){
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

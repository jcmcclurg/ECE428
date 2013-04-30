#include "settings.h"
#include "replica_handler.h"

using namespace mp2;
using namespace std;
using boost::shared_ptr;

#include <boost/format.hpp>
#include <cstdlib>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include "settings.h"

Replica::Replica(int myid, StateMachineFactory & factory, shared_ptr<Replicas> replicas)
		: id(myid), factory(factory), replicas(replicas), 
		  leader(-1), queueLen(0), bwUtilization(0), memUtilization(0),
		  proposalNumber(myid), acceptedProposalNumber(-1), acceptedProposalValue(-1),
		  electionInProgress(false) {

	// any initialization you need goes here
	DEBUG( "Initialized RM " << myid );
}

int16_t Replica::startLeaderElection(void) {
	DEBUG( "RM " << id << " started a leader election.");
	// Send a prepare request to every other replica. Our quorum in this case is considered
	// to be the entire set of replicas. Note that replicas serve all three roles
	// simultaneously. Furthermore, since we are assuming non-Byzantine failures,
	// the distinguished Learner is simply the Replica that begins the leader election process.

	electionInProgress = true;

	int numReplicas = replicas->numReplicas();
	int promisedCount = 0;
	int highestAcceptedValue = id; // Choose myself as the leader if no one objects.

	// Ensures proposal numbers are distinct across all replicas.
	proposalNumber += numReplicas;

	for (int i = 0; i < numReplicas; ++i) {
		if (i != id) {
			Promise promise;
			(*replicas)[i].prepare(promise, proposalNumber);

			if (promise.success) {
				promisedCount++;
				if (promise.acceptedProposalValue > highestAcceptedValue) {
					highestAcceptedValue = promise.acceptedProposalValue;
				}
			}
		}
	}

	// Have enough Acceptors given me their unbreakable word?
	if (promisedCount > numReplicas / 2) {
		proposalNumber += numReplicas;

		// Send out accept requests to all Acceptors.
		int acceptedCount = 0;
		for (int i = 0; i < numReplicas; ++i) {
			if (i != id) {
				bool accepted = (*replicas)[i].accept(proposalNumber, highestAcceptedValue);
				if (accepted) {
					acceptedCount++;
				}
			}
		}

		if (acceptedCount > numReplicas / 2) {
			// Leader elected! 
			leader = highestAcceptedValue;

			// Everyone else assumes a Learner role now.
			for (int i = 0; i < numReplicas; ++i) {
				if (i != id) {
					(*replicas)[i].inform(leader);
				}
			}
		}
	}

	electionInProgress = false;

	return 0;
}

void Replica::prepare(Promise& _return, const int32_t n) {
	electionInProgress = true;

	if (n > highestProposalNumber) {
		highestProposalNumber = n;
		_return.success = true;
		_return.acceptedProposalNumber = acceptedProposalNumber;
		_return.acceptedProposalValue = acceptedProposalValue; 
		return;
	}
	_return.success = false;
	_return.acceptedProposalNumber = -1;
	_return.acceptedProposalValue = -1;
}

bool Replica::accept(const int32_t n, const int32_t value) {
	electionInProgress = true;

	if (n > highestProposalNumber) {
		DEBUG("Accepted value" << n << " for the leader election.");
		acceptedProposalNumber = n;
		acceptedProposalValue = value; 
		return true;
	}
	return false;
}

// Normally, this is sent by Acceptors to Learners. But since we have a single
// distinguished Learner who decides if a majority of the Quorum (all the replicas here)
// has accepted a value, we are guaranteed that this method is only called post-election.
void Replica::inform(const int32_t value) {
	electionInProgress = false;

	leader = value;
}

void Replica::checkExists(const string &name) const throw (ReplicaError) {
	if (machines.find(name) == machines.end()) {
		ReplicaError error;
		error.type = ErrorType::NOT_FOUND;
		error.name = name;
		error.message = string("RM cannot find machine ") + name;
		throw error;
	}
}

void Replica::reshuffleReplicas(const std::string& name, const std::string& val){
	bool done;
	bool found = false;
	do{
		done = true;
		for(int i=0; i< (*replicas).numReplicas(); i++){
			try{
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
			}}catch(ReplicaError){}
		}
	} while(!done);
}

void Replica::createReplicas(const std::string& name, const std::string& val){
	int count;
	do{
		int bm2 = INT_MAX;
		int bmi2;
		count = 0;
		// Find optimal locations to store to
		int i;
		for(i=0; i< (*replicas).numReplicas(); i++){
			try{
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
			}catch(ReplicaError){}
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
		startLeaderElection();
	}
	// Ensure that the leader is still alive.
	else if(leader != id){
		try{
			leader = (*replicas)[leader].getLeader();
		}
		catch(ReplicaError){
			startLeaderElection();
		}
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

int16_t Replica::prepareGetState(int16_t client, const std::string& name){
	if (id != leader){
		ReplicaError error;
		error.type = ErrorType::NOT_LEADER;
		error.name = name;
		error.message = string("Only the leader can prepare a get.");
		throw error;
	}

	// Block until queue does not have any writes for this state.
	DEBUG("Blocking until the queue is free from writes...");
	while(true){
		bool waitingOver = true;
		for(std::vector<int>::size_type i = 0; i != requestQueue[name].size(); i++) {
			if(requestQueue[name][i].first == 'w'){
				waitingOver = false;
				break;
			}
		}
		if(waitingOver){
			break;
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}

	DEBUG("RM " << id << " (leader) responding to a prepare for " << name);
	int bm = INT_MAX;
	int bmi;
	bool success = false;

	// Find the replica with the least bandwidth (for load balancing purposes)
	for(int i=0; i< (*replicas).numReplicas(); i++){
		try{
		if((*replicas)[i].stateExists(name)){
			success = true;
			int b = (*replicas)[i].getBwUtilization();
			if(b < bm){ bmi = i; bm = b; }
		}}
		catch(ReplicaError){}
	}
	if(!success){
		ReplicaError error;
		error.type = ErrorType::NOT_FOUND;
		error.name = name;
		error.message = string("Cannot find machine ") + name;
		throw error;
	}

	// Queue the read request.
	requestQueue[name].push_back(make_pair('r',str((boost::format("%1% %2% %3%") % bmi % client % name))));
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
	DEBUG("RM " << id << " creating " << name);
	queueLen++;
	memUtilization++;
 	machines.insert(make_pair(name, factory.make(initialState)));
	queueLen--;

	// Initialize the queue
	requestQueue[name];// = vector< pair<char,string> >;

	if(leader == id){
		for(int i=0; i< (*replicas).numReplicas(); i++){
			if(i != id){
				try{
					(*replicas)[i].create(name, initialState);
				}catch(ReplicaError){}
			}
		}
	}
}

void Replica::apply(string & result, const string & name, const string& operation) {
	checkExists(name);
	// Enqueue the write
	pair<char,string> p = make_pair('w',str((boost::format("%1%") % operation)));
	requestQueue[name].push_back(p);
	DEBUG("Blocking until my request is at the head of queue...");
	while(true){
		if(requestQueue[name][0] == p){
			break;
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}

	DEBUG("RM " << id << " applying " << name);
	queueLen++;
	bwUtilization++;
	result = machines[name]->apply(operation);
	queueLen--;

	if(leader == id){
		for(int i=0; i< (*replicas).numReplicas(); i++){
			if(i != id){
				try{
					string rslt;
					(*replicas)[i].apply(rslt, name, operation);
				}catch(ReplicaError){}
			}
		}
	}

	// Dequeue the write
	requestQueue.erase(requestQueue.begin());
}

void Replica::notifyFinishedReading(int16_t rmid, int16_t client, const string &name) {
	// Remove it the finished reading from queue.
	bool foundit = false;
	for(std::vector<int>::size_type i = 0; i != requestQueue[name].size(); i++) {
		if(requestQueue[name][i].first == 'r' && requestQueue[name][i].second == str((boost::format("%1% %2% %3%") % rmid % client % name)))
		{
			requestQueue[name].erase(requestQueue[name].begin()+i);
			foundit = true;
			break;
		}
	}
}

void Replica::getState(string& result, int16_t client, const string &name) {
	checkExists(name);
	DEBUG("RM " << id << " getting " << name);
	queueLen++;
	bwUtilization++;
	result = machines[name]->getState();
	queueLen--;

	(*replicas)[leader].notifyFinishedReading(id, client, name);
}

void Replica::remove(const string &name) {
	checkExists(name);
	DEBUG("RM " << id << " removing " << name);
	queueLen++;
	memUtilization--;
	machines.erase(name);
	queueLen--;


	requestQueue.erase(name);

	if(leader == id){
		for(int i=0; i< (*replicas).numReplicas(); i++){
			if(i != id){
				try{
					(*replicas)[i].remove(name);
				}catch(ReplicaError){}
			}
		}
	}
}

/* DO NOT CHANGE THIS */
void Replica::exit(void) {
	clog << "Replica " << id << " exiting" << endl;
	::std::exit(0);	// no return
}

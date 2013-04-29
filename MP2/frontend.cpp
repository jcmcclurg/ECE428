#include "frontend.h"
#include <boost/shared_ptr.hpp>
#include "statemachine.h"
#include "settings.h"

using namespace std;
using namespace mp2;
using boost::shared_ptr;

class StateMachineStub : public mp2::StateMachine {
private:
	ReplicaIf & replica;
	const string name;

public:
	StateMachineStub(ReplicaIf & replica, const string &name)
		: replica(replica), name(name) {}
	virtual string apply(const string & operation) {
		string result;
		replica.apply(result, name, operation);
		return result;
	}

	virtual string getState(void) const {
		string result;
		replica.getState(result, name);
		return result;
	}
};

FrontEnd::FrontEnd(boost::shared_ptr<Replicas> replicas, int i) : replicas(replicas),  id(i) {
	DEBUG("Front end " << id << " started up with access to " << (*replicas).numReplicas() << " replica managers.");
}

FrontEnd::~FrontEnd() { }

shared_ptr<StateMachine> FrontEnd::create(const string &name, const string &initialState) {
	DEBUG("Front end " << id << " creating initial replicas of " << name);
	for(int i = 0; i < (*replicas).numReplicas(); i++){
		(*replicas)[i].create(name, initialState);
	}

	return get(name);
}

shared_ptr<StateMachine> FrontEnd::get(const string &name) {
	shared_ptr<StateMachine> result(new StateMachineStub((*replicas)[0], name));
	return result;
}

void FrontEnd::remove(const string &name) {
	(*replicas)[0].remove(name);
}



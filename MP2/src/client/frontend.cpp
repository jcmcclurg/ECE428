#include <time.h>
#include <boost/shared_ptr.hpp>

#include "frontend.h"
#include "model/statemachine.h"
#include "settings.h"


using namespace std;
using namespace mp2;
using boost::shared_ptr;

class StateMachineStub : public mp2::StateMachine {
private:
	FrontEnd* front;
	const string name;

public:
	StateMachineStub(FrontEnd* e, const string &name)
		: front(e), name(name) {}

	virtual string apply(const string & operation) {
		DEBUG("Stub applying " << name);
		while(true){
			int leader = front->findLeader();
			try{
				string result;
				(*(front->replicas))[leader].apply(result, name, operation);
				return result;
			}catch(ReplicaError){}
		}
	}

	virtual string getState(void) const {
		DEBUG("Stub getting state " << name);
		while(true){
			int leader = front->findLeader();
			string result;
			try{
				int where = (*(front->replicas))[leader].prepareGetState(front->id,name);
				(*(front->replicas))[where].getState(result,front->id,name);
				return result;
			}catch(ReplicaError e){ DEBUG(e.message); }
		}
	}
};

FrontEnd::FrontEnd(boost::shared_ptr<Replicas> replicas, int i) : replicas(replicas),  id(i), leader(-1) {
	DEBUG("FE " << id << " started up with access to " << (*replicas).numReplicas() << " replica managers.");
}

FrontEnd::~FrontEnd() { }

shared_ptr<StateMachine> FrontEnd::create(const string &name, const string &initialState) {
//	while(true){
		findLeader();
//		try{
			DEBUG("FE " << id << " creating state machine " << name);
			(*replicas)[leader].create(name, initialState);
			return get(name);
//		}catch(ReplicaError){}
//	}
}

shared_ptr<StateMachine> FrontEnd::get(const string &name) {
	shared_ptr<StateMachine> result(new StateMachineStub(this, name));
	return result;
}

int FrontEnd::findLeader(void){
	if(leader == -1){
		DEBUG("FE " << id << " looking for a leader");
		for(int i = 0; i < (int)(*replicas).numReplicas()-1; i++){
			try{
				leader = (*replicas)[i].getLeader();
				break;
			}
			catch(ReplicaError){
				// Failed node
			}
		}
		if(leader == -1){
			leader = (*replicas)[(*replicas).numReplicas()-1].getLeader();
		}
	}

	try{
		leader = (*replicas)[leader].getLeader();
	}catch(ReplicaError){
		leader = -1;
		leader = findLeader();
	}

	return leader;
}

void FrontEnd::remove(const string &name) {
	while(true){
		findLeader();
		try{
			(*replicas)[leader].remove(name);
			return;
		}catch(ReplicaError){}
	}
}

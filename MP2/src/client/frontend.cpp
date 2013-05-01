#include <time.h>
#include <boost/shared_ptr.hpp>

#include "frontend.h"
#include "model/statemachine.h"
#include "settings.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

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
		int leader = front->findLeader();
		string result;
		(*(front->replicas))[leader].apply(result, name, operation);
		return result;
	}

	virtual string getState(void) const {
		DEBUG("Stub getting state " << name);
		int leader = front->findLeader();
			string result;
			int where = (*(front->replicas))[leader].prepareGetState(front->id,name);
			(*(front->replicas))[where].getState(result,front->id,name);
			return result;
	}
};

FrontEnd::FrontEnd(boost::shared_ptr<Replicas> replicas, int i) : replicas(replicas),  id(i), leader(-1), findingLeader(false) {
	DEBUG("FE " << id << " started up with access to " << (*replicas).numReplicas() << " replica managers.");
}

FrontEnd::~FrontEnd() { }

shared_ptr<StateMachine> FrontEnd::create(const string &name, const string &initialState) {
		findLeader();
		DEBUG("FE " << id << " creating state machine " << name);
		(*replicas)[leader].create(name, initialState);
		return get(name);
}

shared_ptr<StateMachine> FrontEnd::get(const string &name) {
	shared_ptr<StateMachine> result(new StateMachineStub(this, name));
	return result;
}

int FrontEnd::findLeader(void){
	bool needsNewLeader = false;

	if(leader == -1)
	{
		needsNewLeader = true;
	}
	else{
		// Make sure he's alive
		try{
			int l = (*replicas)[leader].getLeader();
			leader = l;
		}
		catch(ReplicaError e){
			DEBUG("There is a problem with leader " << leader << ": " << e.message);
		}
		catch(...){
			DEBUG("Leader " << leader << " is dead.");
			needsNewLeader = true;
		}
	}

	if(needsNewLeader && !findingLeader){
		findingLeader = true;
		DEBUG("Needs new leader.");
		bool done = false;
		while(!done){
			for(int i = 0; i < (int)(*replicas).numReplicas(); i++){
				try{
					int l = (*replicas)[(*replicas)[i].getLeader()].getLeader();
					leader = l;
					done = true;
					break;
				}
				catch(ReplicaError e){
				DEBUG("RM " << i << " has a problem: " << e.message);
				}
				catch(...){
				DEBUG("RM " << i << " is dead.");
				}
			}
			if(!done){
				boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			}
		}
		findingLeader = false;
	}
	else if(needsNewLeader){
		DEBUG("Waiting for new leader.");
		while(findingLeader){
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
		}
	}
	else{
		DEBUG("Leader " << leader << " is alive.");
	}
	return leader;
}

void FrontEnd::remove(const string &name) {
	findLeader();
	(*replicas)[leader].remove(name);
}

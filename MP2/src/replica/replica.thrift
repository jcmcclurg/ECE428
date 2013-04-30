namespace cpp mp2

enum ErrorType {
	NOT_FOUND,
	ALREADY_EXISTS,
	NOT_LEADER
}

exception ReplicaError {
	1: ErrorType type,
	2: string name, 	// state machine name for the operation
	3: string message	// formatted error
}

struct Promise {
	1: bool success,
	2: i32 acceptedProposalNumber,
	3: i32 acceptedProposalValue
}

service Replica {
	// create a state machine
	void create(1:string name, 2:string initialState) throws (1:ReplicaError e),

	// apply an operation on a state machine
	string apply(1:string name, 2:string operation) throws (1:ReplicaError e),

	// get the state of a state machine
	string getState(1: string name) throws (1:ReplicaError e),

	// remove a state machine
	void remove(1: string name) throws (1:ReplicaError e),

	// ask about RM info
	i16 prepareGetState(1: string name) throws (1:ReplicaError e),
	i16 getLeader(),
	i16 getQueueLen(),
	i16 getBwUtilization(),
	i16 getMemUtilization(),
	i16 startLeaderElection(),
	bool stateExists(1: string name),

	// paxos
	Promise prepare(1: i32 n),
	bool accept(1: i32 n, 2: i32 value),
	oneway void inform(1: i32 value),

	// exit / crash
	oneway void exit()
}

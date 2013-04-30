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

	// apply an operation on a state machine. blocks until lock is released
	string apply(1:string name, 2:string operation) throws (1:ReplicaError e),

	// get the state of a state machine. you must specify the client number
	string getState(1: i16 client, 2: string name) throws (1:ReplicaError e),

	// remove a state machine
	void remove(1: string name) throws (1:ReplicaError e),

	// called by clients to find out which RM they should call getState() on
	i16 prepareGetState(1: i16 client, 2: string name),

	// Leader-RM communication
	// returns the current leader
	i16 getLeader(),
	i16 getQueueLen(),
	i16 getBwUtilization(),
	i16 getMemUtilization(),
	i16 startLeaderElection(),
	bool stateExists(1: string name),

	// RM uses this to tell the leader it's finished reading
	void notifyFinishedReading(1: i16 rmid, 2: i16 client, 3: string name),

	// paxos
	Promise prepare(1: i32 n),
	bool accept(1: i32 n, 2: i32 value),
	oneway void inform(1: i32 value),

	// exit / crash
	oneway void exit()
}
/*
enum ReplicaMode {
	FREE,
	READING,
	WAITING
}
	// get a comma-separated list of states
	string getStates(),

	// tell one RM to copy a state to another RM
	void copyState(1:i16 from 2:i16 to, string name) throws (1:ReplicaError e),

	// get the current mode of the replica
	ReplicaMode getMode(),
	// called by leader to notify the RM that he should expect a read from a client
	i16 readyGetState(1: string name, 2: i16 waitFor) throws (1:ReplicaError e),*/

all: chat

SOURCEFILES = unicast.cc mcast.cc chat.cc
HEADERS = mp1.h

operators.o: operators.cc
	g++ -g -c $^

message.o: message/message.cc
	g++ -g -c $^

timestamp.o: timestamp/timestamp.cc 
	g++ -g -c $^

heartbeat.o: heartbeat/heartbeat.cc
	g++ -g -c $^

state.o: state/state.cc
	g++ -g -c $^

debug: $(SOURCEFILES) $(HEADERS) operators.h operators.o message.o timestamp.o heartbeat.o state.o
	g++ -g -pthread -o $@ $(SOURCEFILES) -DDEBUG message.o timestamp.o heartbeat.o state.o -lrt

chat: $(SOURCEFILES) $(HEADERS) state.o message.o timestamp.o heartbeat.o
	g++ -g -pthread -o $@ $(SOURCEFILES) message.o timestamp.o heartbeat.o state.o -lrt

test: test.cc message.o timestamp.o heartbeat.o state.o
	g++ -g -pthread -o $@ test.cc message.o timestamp.o heartbeat.o state.o -lrt

clean:	restart
	-rm -f chat test *.o *~

restart:
	-rm -f GROUPLIST

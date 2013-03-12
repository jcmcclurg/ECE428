SOURCEFILES = unicast.cc mcast.cc chat.cc
HEADERS = mp1.h
OFILES = operators.o message.o timestamp.o heartbeat.o state.o

all: chat
DEBUG=

debugr: restart debug

debug: chat
DEBUG=-DDEBUG

operators.o: operators.cc
	g++ $(DEBUG) -g -c $^

message.o: message/message.cc
	g++ $(DEBUG) -g -c $^

timestamp.o: timestamp/timestamp.cc 
	g++ $(DEBUG) -g -c $^

heartbeat.o: heartbeat/heartbeat.cc
	g++ $(DEBUG) -g -c $^

state.o: state/state.cc
	g++ $(DEBUG) -g -c $^

chat: $(SOURCEFILES) $(HEADERS) $(OFILES)
	g++ $(DEBUG) -g -pthread -o $@ $(SOURCEFILES) $(OFILES) -lrt

test: test.cc $(OFILES)
	g++ $(DEBUG) -g -pthread -o $@ test.cc $(OFILES) -lrt

clean:	restart
	-rm -f chat test *.o *~

restart:
	-rm -f GROUPLIST

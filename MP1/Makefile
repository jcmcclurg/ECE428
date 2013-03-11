all: chat

SOURCEFILES = unicast.cc mcast.cc chat.cc
HEADERS = mp1.h

message.o: message/message.cc
	g++ -g -c $^

timestamp.o: timestamp/timestamp.cc 
	g++ -g -c $^

heartbeat.o: heartbeat/heartbeat.cc
	g++ -g -c $^

debug: $(SOURCEFILES) $(HEADERS) message.o timestamp.o heartbeat.o
	g++ -g -pthread -o $@ $(SOURCEFILES) -DDEBUG message.o timestamp.o heartbeat.o -lrt

chat: $(SOURCEFILES) $(HEADERS) message.o timestamp.o heartbeat.o
	g++ -g -pthread -o $@ $(SOURCEFILES) message.o timestamp.o heartbeat.o -lrt

test: test.cc message.o timestamp.o heartbeat.o
	g++ -g -pthread -o $@ test.cc message.o timestamp.o heartbeat.o -lrt

clean:	restart
	-rm -f chat test *.o *~

restart:
	-rm -f GROUPLIST

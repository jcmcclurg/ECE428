all: chat

SOURCEFILES = unicast.cc mcast.cc chat.cc
HEADERS = mp1.h

delivery: delivery.cc
	g++ ${CFLAGS} -g -o $@ $^

message: message.cc
	g++ ${CFLAGS} -g -o $@ $^

timestamp: timestamp.cc
	g++ ${CFLAGS} -g -o $@ $^

chat: $(SOURCEFILES) $(HEADERS)
	g++ ${CFLAGS} -g -pthread -o $@ $(SOURCEFILES)

clean:	restart
	-rm -f chat *.o *~

restart:
	-rm -f GROUPLIST

#!/usr/bin/python

from subprocess import Popen,PIPE   
import time
from select import select   
from random import expovariate
import re
import sys
                       
if len(sys.argv) < 2:
    print "Usage: %s rate [delay]" % sys.argv[0]
    print " rate is the number of messages to send per second on average"
    print " delay is how long to sleep before sending the first message (default 10s)"
    sys.exit(1)
rate = float(sys.argv[1])       # messages per second
if len(sys.argv) > 2:
    initsleep = float(sys.argv[2]) 
else:
    initsleep = 10

p = Popen("./chat", stdin=PIPE, stdout=PIPE, stderr=PIPE)

x = p.stderr.readline()
                        
opn = "Our port number: "    
assert x[:len(opn)] == opn
ourport = int(x[len(opn):].strip())  
                        
print "Got our port: ", ourport

print "Sleeping for %ss" % initsleep
           
state = "INITSLEEP"
deadline = time.time() + initsleep   
enddeadline = deadline + 30     # run for 30s       
members = [] 
messages = []

while True:  
    waittime = deadline - time.time()
    if waittime > 0:
        rready, _, _ = select([p.stdout,p.stderr],[],[],waittime)
    else:
        rready = []
    if not rready:
        if state == "INITSLEEP":
            print "Starting sending messages"
            members.sort()
            myidx = members.index(ourport)    
            timestamp = [0 for m in members]
            state = "NORMAL"
            # fall through
        if state == "NORMAL":      
            if time.time() < enddeadline:
                deadline = time.time() + expovariate(rate)
                timestamp[myidx] += 1                   
                print "Sending message %s" % timestamp
                p.stdin.write("TS %s\n" % timestamp)
            else:                                       
                print "Not sending.";
                state = "FINISHED"                               
                deadline = time.time() + 30     # 30s quiet period
        if state == "FINISHED" and time.time() > enddeadline + 60:
            break
    else:
        if p.stdout in rready:   
            l = p.stdout.readline()
            m = re.match(r'<(\d+)> TS \[((\d+,\s*)*\d+)\]',l)
            if m:        
                if state == "INITSLEEP":
                    # no need to wait anymore
                    members.sort()
                    myidx = members.index(ourport)    
                    timestamp = [0 for _ in members]
                    state = "NORMAL"
                source = int(m.group(1))  
                rects = map(int, m.group(2).split(', '))  
                print "Received %s from %s" % (rects, source)
                assert rects[myidx] <= timestamp[myidx]
                for m in messages:
                    if all([x <= y for x,y in zip(rects, m)]):
                        print "!!! ERROR: Previous timestamp %s is >= received timestamp %s" % (m, rects)  
                messages.append(rects)
                timestamp = [max(x,y) for x,y in zip(rects,timestamp)]
            else:
                print "| "+l.strip()
        if p.stderr in rready:
            l = p.stderr.readline()
            m = re.match("New group member: (\d+)",l)
            if m:                 
                assert state == "INITSLEEP"
                members.append(int(m.group(1)))
                print "Added member %d" % members[-1]
            else:
                print "# "+l.strip()
                
print "Exiting"

p.kill()

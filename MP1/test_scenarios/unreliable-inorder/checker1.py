#!/usr/bin/env python

import pdb
from difflib import unified_diff
import re
import optparse
import collections
import multiprocessing
import os
import subprocess
import sys
import time



# this is like scenario in hw 2, except we drop p4's first msg to
# p3. also, to prevent early-re-requests of missed msgs from
# interferrin with our expected transcripts, we give max delay to
# re-sent msgs.

expectedtranscriptlines = {
    'p1':
    '''<p1> T3xM3u6w_b:   hello this is p1 _1000_   N81XrBd3 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 again _2000_   05MFsLNv T3xM3u6w_e
<p2> T3xM3u6w_b:   this is p2 _0100_   q2enK/vL T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is 3rd msg from p1 _3100_   n6lh47Xw T3xM3u6w_e
<p3> T3xM3u6w_b:   greetings from p3 _2110_   Rm+KVVL4 T3xM3u6w_e
<p4> T3xM3u6w_b:   msg from p4 _0101_   tS86sjtS T3xM3u6w_e
<p4> T3xM3u6w_b:   this is 2nd from p4 _3112_   WaUZ8gmY T3xM3u6w_e
'''.splitlines()
    ,

    'p2':
    '''<p2> T3xM3u6w_b:   this is p2 _0100_   q2enK/vL T3xM3u6w_e
<p4> T3xM3u6w_b:   msg from p4 _0101_   tS86sjtS T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 _1000_   N81XrBd3 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 again _2000_   05MFsLNv T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is 3rd msg from p1 _3100_   n6lh47Xw T3xM3u6w_e
<p3> T3xM3u6w_b:   greetings from p3 _2110_   Rm+KVVL4 T3xM3u6w_e
<p4> T3xM3u6w_b:   this is 2nd from p4 _3112_   WaUZ8gmY T3xM3u6w_e
'''.splitlines()

    # if following "queue" to hold msgs, then p2 should deliver 2110
    # before 3100 (2110 arrives before 3100), but delivering 3100
    # before 2110 is also causally valid. should we deduct points for
    # this?
    ,

    'p3':
    '''<p1> T3xM3u6w_b:   hello this is p1 _1000_   N81XrBd3 T3xM3u6w_e
<p2> T3xM3u6w_b:   this is p2 _0100_   q2enK/vL T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 again _2000_   05MFsLNv T3xM3u6w_e
<p3> T3xM3u6w_b:   greetings from p3 _2110_   Rm+KVVL4 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is 3rd msg from p1 _3100_   n6lh47Xw T3xM3u6w_e
<p4> T3xM3u6w_b:   msg from p4 _0101_   tS86sjtS T3xM3u6w_e
<p4> T3xM3u6w_b:   this is 2nd from p4 _3112_   WaUZ8gmY T3xM3u6w_e
'''.splitlines()
    ,

    'p4':
    '''<p2> T3xM3u6w_b:   this is p2 _0100_   q2enK/vL T3xM3u6w_e
<p4> T3xM3u6w_b:   msg from p4 _0101_   tS86sjtS T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 _1000_   N81XrBd3 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 again _2000_   05MFsLNv T3xM3u6w_e
<p3> T3xM3u6w_b:   greetings from p3 _2110_   Rm+KVVL4 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is 3rd msg from p1 _3100_   n6lh47Xw T3xM3u6w_e
<p4> T3xM3u6w_b:   this is 2nd from p4 _3112_   WaUZ8gmY T3xM3u6w_e
'''.splitlines()
    ,

}

# creates processes and feed them commands
cmds = '''
[p1]
wait 1
#
# tx at 1, rx at (-, x, 2, 10): dropped on way to p2, and we expect
# p2 will request after seeing p1 2nd msg, since p3 and p4 msgs to p2
# wont reflect p1 1st msg
send T3xM3u6w_b:   hello this is p1 _1000_   N81XrBd3 T3xM3u6w_e
#
wait 2
#
# tx at 3, rx at (-, 11, 8, 11)
send T3xM3u6w_b:   hello this is p1 again _2000_   05MFsLNv T3xM3u6w_e
#
wait 8
#
# tx at 11, rx at (-, 13, 16, 13)
send T3xM3u6w_b:   hello this is 3rd msg from p1 _3100_   n6lh47Xw T3xM3u6w_e
#
wait 20
# time 31

[p2]
wait 3
#
# tx at 3, rx at (4, -, 6, 4)
send T3xM3u6w_b:   this is p2 _0100_   q2enK/vL T3xM3u6w_e
#
wait 27

[p3]
wait 11
#
# tx at 11, rx at (14, 14, -, 12)
send T3xM3u6w_b:   greetings from p3 _2110_   Rm+KVVL4 T3xM3u6w_e
#
wait 20

[p4]
wait 7
#
# tx at 7, rx at (15, 8, x, -): dropped on way to p3, and we expect
# p3 will request after seeing p4 2nd msg, since p1 and p2 msgs to p3
# wont reflect p4 1st msg
send T3xM3u6w_b:   msg from p4 _0101_   tS86sjtS T3xM3u6w_e
#
wait 7
#
# tx at 14, rx at (18, 18, 18, -)
send T3xM3u6w_b:   this is 2nd from p4 _3112_   WaUZ8gmY T3xM3u6w_e
#
wait 15
'''

delaydropspecs = {
    'p1':
    '|'.join(['T3xM3u6w_b:   hello this is p1 _1000_   N81XrBd3 T3xM3u6w_e',
              '0', '-1', '1000000', '9000000']) + '\n'
    +
    '|'.join(['T3xM3u6w_b:   hello this is p1 again _2000_   05MFsLNv T3xM3u6w_e',
              '0', '8000000', '5000000', '8000000']) + '\n'
    +
    '|'.join(['T3xM3u6w_b:   hello this is 3rd msg from p1 _3100_   n6lh47Xw T3xM3u6w_e',
              '0', '2000000', '5000000', '2000000']) + '\n'
    ,

    'p2':
    '|'.join(['T3xM3u6w_b:   this is p2 _0100_   q2enK/vL T3xM3u6w_e',
              '1000000', '0', '3000000', '1000000']) + '\n'
    ,

    'p3':
    '|'.join(['T3xM3u6w_b:   greetings from p3 _2110_   Rm+KVVL4 T3xM3u6w_e',
              '3000000', '3000000', '0', '1000000']) + '\n'
    ,

    'p4':

    # this is dropped on the way to p3
    '|'.join(['T3xM3u6w_b:   msg from p4 _0101_   tS86sjtS T3xM3u6w_e',
              '8000000', '1000000', '-1', '0']) + '\n'
    +
    '|'.join(['T3xM3u6w_b:   this is 2nd from p4 _3112_   WaUZ8gmY T3xM3u6w_e',
              '4000000', '4000000', '4000000', '0']) + '\n'
    ,
    }


class dummy_obj:
    pass

def parse_file(f):
    commands = {'wait': 2, 'send': 2, 'crash': 1}
    output = []
    current = None
    for line in f.readlines():
        line = line.strip().split(None, 1)
        if len(line) == 0 or line[0].startswith('#'):
            # empty or a comment line
            continue
        if (line[0].startswith('[')):
            try:
                process_name = line[0][1:-1]
                current = (process_name, list())
                output.append(current)
            except:
                print "=> Name extraction failure"
                raise
        elif commands.has_key(line[0]):
            current[1].append(line)
        else:
            print "=> Unknown command %s with args %s" % tuple(line)
            sys.exit(1)

    return output

def run_cmds(command_list, name, pipe):
    commands = {'wait': lambda x: time.sleep(float(x)),
                'send': lambda x: p.stdin.write('%s\n' % x),
                'quit': lambda x: p.stdin.write('/quit'),
                'crash': lambda x: p.terminate(),
                }
    stderrfile=open('%s.stderr' % name, 'w')
    # launch the process...
    p = subprocess.Popen(['./chat', '--delaydropspec', 'delaydropspec.' + name],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=stderrfile)
    time.sleep(2)

    # read the first line, which should be 'Our port number: <...>',
    # to extract the our group member id (port number) to send to the
    # main test harness process
    line = p.stdout.readline()
    match = re.match(re.compile(r'Our port number: ([0-9]+)'), line)
    assert match
    memberid = match.group(1)
    pipe.send(memberid)

    # the second line
    assert (p.stdout.readline() == 'done with delaydropspec\n')

    # now wait until we receive a "start" msg from main harness
    # process before starting to run commands
    assert('start' == pipe.recv())
    print time.ctime(), 'member %s (id %s) starts processing commands' % (name, memberid)
    try:
        for command, arg in command_list:
            print (time.ctime()), 'member %s (id %s) runs: %s' % (
                name, memberid, ' '.join([command, arg]))
            commands[command](arg)
            pass
        pass
    except Exception as e:
        print time.ctime(), 'member %s (id %s) has problems with commands: %s' % (name, memberid, str(e))
        pass

    if not p.poll() is None:
        try:
            p.terminate()
            pass
        except Exception as e:
            print time.ctime(), 'member %s (id %s) has problems terminating: %s' % (name, memberid, str(e))
            pass
        pass
    stdoutdata, _ = p.communicate()
    # write stderr to file
    pipe.send(stdoutdata)
    pipe.close()
    stderrfile.close()
    
#    print 'group member %s (pid %d) closed' %(memberid, p.pid)

def main():
    #parser = argparse.ArgumentParser('Runs test instances of ./chat.')
    #parser.add_argument('infile', type=file)
    #parser.add_argument('-o', '--outfile', type=file, default=open('test.out.txt', 'w')
    #args = parser.parse_arguments()
    parser = optparse.OptionParser()
    parser.add_option('-o', '--outfile')
    options, o_args = parser.parse_args()

    args = dummy_obj()

    if len(o_args) == 2:
        args.infile = open(o_args[0], 'r')
        pass
    else:
        import StringIO
        global cmds
        args.infile = StringIO.StringIO(cmds)
        pass
    if options.outfile == None:
        args.outfile = open('test.out.txt', 'w')
    else:
        args.outfile = open(options.outfile, 'w')

    commands = parse_file(args.infile)
    names = [x[0] for x in commands]

    if os.path.exists('GROUPLIST'):
        os.remove('GROUPLIST')

    processes = {}
    for name, cmds in commands:
        path = 'delaydropspec.' + name
        f = open(path, 'w')
        f.write(delaydropspecs[name])
        f.close()

        processes[name] = dummy_obj()
        processes[name].pipe = multiprocessing.Pipe()
        processes[name].process = (multiprocessing.Process(target=run_cmds,
              args=(cmds, name, processes[name].pipe[1])))
        processes[name].process.start()
        time.sleep(1)
        processes[name].memberid = int(processes[name].pipe[0].recv())

    # send the 'start' cmd to the python subprocesses
    starttime = int(time.time())
    for name in names:
        processes[name].pipe[0].send('start')
    # Wait for all processes to finish
    for name in names:
        processes[name].process.join()
        # expect the child to have written the stderr file
        assert (os.path.exists('%s.stderr' % name))
        # shift logged times, to make relative to starttime
        f = open('%s.stderr' % name)
        data = f.read()
        f.close()
        # write out massaged data
        f = open('%s.stderr' % name, 'w')
        for line in data.splitlines():
            match = re.match(re.compile(r'^debug: <([0-9]+)> time= ([0-9]+),'),
                             line)
            if match:
                line = line.replace(
                    'debug: <%s> time= %s,' % (match.groups()),
                    'debug: <%s> time= %2d,' % (name, int(match.group(2))-starttime))
                for _name in names:
                    line = line.replace('<%s>' % processes[_name].memberid,
                                        '<%s>' % _name)
                    pass
                pass
            f.write(line + '\n')
            pass
        f.close()
        pass

    error = False

    for name in names:
        args.outfile.write('=' * 8 + ' ' + name + ' ' + '=' * 8 + '\n')
        data = processes[name].pipe[0].recv()
        # replace member ids with member names
        for _name in names:
            data = data.replace('<%s>' % processes[_name].memberid,
                                '<%s>' % _name)
            pass
        args.outfile.write(data)
        diff = '\n'.join(unified_diff(expectedtranscriptlines[name],
                                      data.splitlines(),
                                      fromfile='expected',
                                      tofile='yours'))
        if len(diff) > 0:
            print
            print '****** FAIL: output of %s not as expected.' % (name)
            print 'showing diff -u <expected> <yours>:'
            print diff
            print '---------------- (end diff)'
            error = True
            pass
        f = open('%s.stdout' % name, 'w')
        f.write(data)
        f.close()
        pass

    if error:
        print 'FAIL: there was some error'
        pass
    else:
        print 'PASS'
        pass
    os.remove('GROUPLIST')

if __name__ == '__main__':
    main()

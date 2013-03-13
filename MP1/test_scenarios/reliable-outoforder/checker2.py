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



# this is like scenario in hw 2 with a little modification


expectedtranscriptlines = {
    'p1':
    '''<p3> T3xM3u6w_b:   greetings from p3 _0010_ W91XeGg9 T3xM3u6w_e
<p4> T3xM3u6w_b:   that is funny yo _0001_ Br/r75bn T3xM3u6w_e
<p3> T3xM3u6w_b:   2nd msg from p3 _0021_ yNFB8aD2 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 _1021_ KgIenK7I T3xM3u6w_e
<p3> T3xM3u6w_b:   it is p3 again _1031_ gYA8P7zv T3xM3u6w_e
<p2> T3xM3u6w_b:   first msg from p2 _1131_ N9iVQFcG T3xM3u6w_e
<p3> T3xM3u6w_b:   yet again p3 is talking _1041_ AIQ2gGiV T3xM3u6w_e
'''.splitlines()
    ,

    'p2':
    '''<p3> T3xM3u6w_b:   greetings from p3 _0010_ W91XeGg9 T3xM3u6w_e
<p4> T3xM3u6w_b:   that is funny yo _0001_ Br/r75bn T3xM3u6w_e
<p3> T3xM3u6w_b:   2nd msg from p3 _0021_ yNFB8aD2 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 _1021_ KgIenK7I T3xM3u6w_e
<p3> T3xM3u6w_b:   it is p3 again _1031_ gYA8P7zv T3xM3u6w_e
<p2> T3xM3u6w_b:   first msg from p2 _1131_ N9iVQFcG T3xM3u6w_e
<p3> T3xM3u6w_b:   yet again p3 is talking _1041_ AIQ2gGiV T3xM3u6w_e
'''.splitlines()
    ,

    'p3':
    '''<p3> T3xM3u6w_b:   greetings from p3 _0010_ W91XeGg9 T3xM3u6w_e
<p4> T3xM3u6w_b:   that is funny yo _0001_ Br/r75bn T3xM3u6w_e
<p3> T3xM3u6w_b:   2nd msg from p3 _0021_ yNFB8aD2 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 _1021_ KgIenK7I T3xM3u6w_e
<p3> T3xM3u6w_b:   it is p3 again _1031_ gYA8P7zv T3xM3u6w_e
<p3> T3xM3u6w_b:   yet again p3 is talking _1041_ AIQ2gGiV T3xM3u6w_e
<p2> T3xM3u6w_b:   first msg from p2 _1131_ N9iVQFcG T3xM3u6w_e
'''.splitlines()
    ,

    'p4':
    '''<p4> T3xM3u6w_b:   that is funny yo _0001_ Br/r75bn T3xM3u6w_e
<p3> T3xM3u6w_b:   greetings from p3 _0010_ W91XeGg9 T3xM3u6w_e
<p3> T3xM3u6w_b:   2nd msg from p3 _0021_ yNFB8aD2 T3xM3u6w_e
<p1> T3xM3u6w_b:   hello this is p1 _1021_ KgIenK7I T3xM3u6w_e
<p3> T3xM3u6w_b:   it is p3 again _1031_ gYA8P7zv T3xM3u6w_e
<p2> T3xM3u6w_b:   first msg from p2 _1131_ N9iVQFcG T3xM3u6w_e
<p3> T3xM3u6w_b:   yet again p3 is talking _1041_ AIQ2gGiV T3xM3u6w_e
'''.splitlines()

    # if following "queue" to hold msgs, then p4 should deliver 1131
    # before 1041 (1131 arrives before 1041), but delivering 1041
    # before 1131 is also causally valid. should we deduct points for
    # this?
    ,

}

# creates processes and feed them commands
cmds = '''
[p1]
wait 7
#
# tx at 7, rx at (-, 8, 8, 16)
send T3xM3u6w_b:   hello this is p1 _1021_ KgIenK7I T3xM3u6w_e
#
wait 10


[p2]
wait 11
#
# tx at 11, rx at (12, -, 14, 12)
send T3xM3u6w_b:   first msg from p2 _1131_ N9iVQFcG T3xM3u6w_e
#
wait 7

[p3]
wait 1
#
# tx at 1, rx at (2, 2, -, 4)
send T3xM3u6w_b:   greetings from p3 _0010_ W91XeGg9 T3xM3u6w_e
#
wait 2
#
# tx at 3, rx at (5, 5, -, 6)
send T3xM3u6w_b:   2nd msg from p3 _0021_ yNFB8aD2 T3xM3u6w_e
#
wait 6
#
# tx at 9, rx at (10, 7, -, 12)
send T3xM3u6w_b:   it is p3 again _1031_ gYA8P7zv T3xM3u6w_e
#
wait 3
#
# tx at 12, rx at (16, 16, -, 14)
send T3xM3u6w_b:   yet again p3 is talking _1041_ AIQ2gGiV T3xM3u6w_e
#
# now is time 12
wait 6

[p4]
wait 1
#
# tx at 1, rx at (6, 6, 2, -)
send T3xM3u6w_b:   that is funny yo _0001_ Br/r75bn T3xM3u6w_e
#
wait 17
'''

delaydropspecs = {
    'p1':
    '|'.join(['T3xM3u6w_b:   hello this is p1 _1021_ KgIenK7I T3xM3u6w_e',
              '0', '1000000', '1000000', '9000000']) + '\n'
    ,


    'p2':
    '|'.join(['T3xM3u6w_b:   first msg from p2 _1131_ N9iVQFcG T3xM3u6w_e',
              '1000000', '0', '3000000', '1000000']) + '\n'
    ,


    'p3':
    '|'.join(['T3xM3u6w_b:   greetings from p3 _0010_ W91XeGg9 T3xM3u6w_e',
              '1000000', '1000000', '0', '3000000']) + '\n'
    +
    '|'.join(['T3xM3u6w_b:   2nd msg from p3 _0021_ yNFB8aD2 T3xM3u6w_e',
              '2000000', '2000000', '0', '3000000']) + '\n'
    +
    '|'.join(['T3xM3u6w_b:   it is p3 again _1031_ gYA8P7zv T3xM3u6w_e',
              '4000000', '1000000', '0', '6000000']) + '\n'
    +
    '|'.join(['T3xM3u6w_b:   yet again p3 is talking _1041_ AIQ2gGiV T3xM3u6w_e',
              '4000000', '4000000', '0', '2000000']) + '\n'
    ,


    'p4':
    '|'.join(['T3xM3u6w_b:   that is funny yo _0001_ Br/r75bn T3xM3u6w_e',
              '5000000', '5000000', '1000000', '0']) + '\n'
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

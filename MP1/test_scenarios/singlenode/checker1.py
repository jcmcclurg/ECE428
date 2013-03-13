#!/usr/bin/env python

#
#
# just a single node sending 50 msgs, 1 second apart
#
#




from difflib import unified_diff
import re
import optparse
import collections
import multiprocessing
import os
import subprocess
import sys
import time

class dummy_obj:
    pass



expectedtranscriptlines = {
    'p1':
    map(lambda num: '<p1> this is msg number %d' % num,
                              range(50))
    }

cmds = '''
[p1]
''' + '\n'.join(map(lambda num: 'wait 1\nsend this is msg number %d' % num,
                    range(50)))
cmds += '\nwait 5\n'

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
    p = subprocess.Popen('./chat', stdin=subprocess.PIPE, stdout=subprocess.PIPE,
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

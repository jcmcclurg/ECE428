#!/usr/bin/env python
import subprocess
import signal
import sys

def signal_handler(s, f):
  r0.send_signal(signal.SIGINT)
  r1.send_signal(signal.SIGINT)
  r2.send_signal(signal.SIGINT)

  print 'Exiting!'
  sys.exit(0)

def flush_streams(proc):
    pipe_data = proc.communicate()
    for data in pipe_data:
      print data

def monitor_procs(procs):
  while proc.returncode == None:
    flush_streams(proc, stdout_log, stderr_log)
  flush_streams(proc, stdout_log, stderr_log)

signal.signal(signal.SIGINT, signal_handler)

r0 = subprocess.Popen(["./replica", "0"], cwd="../bin")
r1 = subprocess.Popen(["./replica", "1"], cwd="../bin")
r2 = subprocess.Popen(["./replica", "2"], cwd="../bin")

signal.pause()

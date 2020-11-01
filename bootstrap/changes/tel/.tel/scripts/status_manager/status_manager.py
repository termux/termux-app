#!/usr/bin/env python
#Asynchronous status manager / script loader - sealyj 2020
import subprocess
import threading
import sys
import os
import queue
import signal
from blessed import Terminal
from time import sleep #for battery saving


#globals and definitions
term = Terminal(force_styling=True) #force required if output not a tty
homedir = os.path.expanduser("~")
#text_path = homedir + "/.tel/data/notifications"
status_scripts_dir = homedir + "/.tel/status"
status_scripts = []

#set work dir
os.chdir(homedir + "/.tel/scripts/status_manager")

#import env vars
status_enabled = os.environ['STATUS_WINDOW_ENABLED']
user_sleeptime = float(os.environ['STATUS_MANAGER_SLEEP'])
#center_output = os.environ['CENTER_STATUS']
notifications_enabled = os.environ['NOTIFICATIONS_ENABLED']
#tel_version = os.environ['TEL_VERSION']
#status_manager_version = os.environ['STATUS_MANAGER_VERSION']

# set window name for target resizing
#os.system("tmux rename-window 'Status_Manager'")
# immediately resize
#os.system("tmux resize-pane -t 1.top -y 5")


#print(term.center("Status Manager Version: " + str(status_manager_version)))
#os.system("clear")
print(term.clear)
#currently used instead of signal
def check_terminal(prev_status_bar, last_measured):
    size = term.height #, term.width
   # print("term size is : " + str(size)
    if size != last_measured:
        #os.system("clear")
        if term.height != len(status_scripts):
            os.system("tmux resizep -t 1.top -y {}".format(len(status_scripts + 1)))
            print(term.clear + term.home)
            for line_num in range(0,len(status_scripts) + 1):
                print(term.home + term.move_y(line_num) + term.clear_eol + term.center(prev_status_bar[line_num]))
    return size

#currenty unused
def on_resize(sig, action):
  #  print(f'height={term.height}, width={term.width}')
    resized_status_bar = prev_status_bar
    print(term.home + term.clear)
    for line_num in range(len(prev_status_bar)):
       print(term.home + term.move_xy(0,line_num) + term.clear_eol + term.center(prev_status_bar[line_num]))

def get_scripts():
    status_scripts_list = []
    for root, dirs, files in os.walk(status_scripts_dir, topdown=True):
        for name in files:
            if name.endswith('.sh') or name.endswith('.py'):
                status_scripts_list.append(os.path.join(root, name)) 
                if root.count(os.sep) - status_scripts_dir.count(os.sep) == 0:
                    del dirs[:]
    status_scripts_list.sort()
    return status_scripts_list

def init_status_bar(status_scripts):
    status_bar = []
    for n in range(0,len(status_scripts)):
        status_bar.append("Script " + str(n) + " Loading..." )
    return status_bar

def store_complete_status_bar(prev_status_bar):
    term.clear
    for item in prev_status_bar:
        print(item)
    term.clear
    return prev_status_bar

def print_status_bar(prev_status_bar,script_num,new_output):
    status_bar = prev_status_bar
    for line in range(0,len(prev_status_bar)):
        if line == script_num:
            if new_output != prev_status_bar[line]:
               #replace
                status_bar[line] = new_output
                if len(new_output) > term.width:
                    if line == len(prev_status_bar):
                        print(term.move_y(line) + term.clear_eol + term.wrap(new_output))
                    else:
                        new_output = new_output[:term.width -2]
                        new_output = new_output + term.normal + '..'
                        print(term.move_y(line) + term.clear_eol + term.center(new_output))

                else:
                    print(term.move_y(line) + term.clear_eol + term.center(new_output))
    return status_bar

#read output from status script wrappers stdout, when line has been read, puts it into queue
def read_output(pipe, q):
    while True:
        l = pipe.readline()
        q.put(l)

#############main starts here############

#get status scripts and init status bar
status_scripts = get_scripts()
empty_status_bar = init_status_bar(status_scripts)
status_bar = empty_status_bar
all_procs = []
# start all scripts as subprocesses
for script_num in range(0,len(status_scripts)):
    new_process = subprocess.Popen(["./status_script_wrapper.sh", status_scripts[script_num], str(term.width)], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    all_procs.append(new_process)
   # print('Loading: ' + str(status_scripts[script_num]))   
proc_queues = []
# create queue for storing each scripts output
for script_num in range(0,len(status_scripts)):
    create_queue = queue.Queue()
    proc_queues.append(create_queue)


proc_threads = []
# create threads
for script_num in range(0,len(status_scripts)):
    create_thread = threading.Thread(target=read_output, args=(all_procs[script_num].stdout, proc_queues[script_num]))
    proc_threads.append(create_thread)

# start threads to read output from processes
for script_num in range(0,len(status_scripts)):
    proc_threads[script_num].daemon = True
    proc_threads[script_num].start()
main_loop = 0
last_measured = term.height, term.width #initial measurement required to pass to func
stored_bar = status_bar

for n in range(0,len(status_scripts)):
        new_output = ("Loading.. " + status_scripts[n].split("/status/",1)[1])
        prev_status_bar = print_status_bar(status_bar, n, new_output)

while True:

    #if window sized changed clear
 #   main_loop = main_loop + 1
 #   if main_loop == 5:
    last_measured = check_terminal(prev_status_bar, last_measured)
 #       main_loop = 0
    #signal.signal(signal.SIGWINCH, on_resize)

    for process in range(0,len(all_procs)):
        all_procs[process].poll()
    # check if any subprocess has finished
    
    for process in range(0,len(all_procs)):
        if all_procs[process].returncode is not None:
        #    print(str(all_procs[process].returncode))
            break
    new_output = None
    prev_status_bar = status_bar
    for process in range(0,len(proc_queues)):
        try:
            new_output = None
            new_output = proc_queues[process].get(False)
            #if new_output:
            new_output = new_output.decode('utf-8').rstrip('\r|\n')
            prev_status_bar = print_status_bar(prev_status_bar, process, new_output)

        #    if term.height != len(status_scripts) + 1:
         #       os.system("tmux resizep -t 1.top -y {}".format(len(prev_status_bar) + 1 ))
        except queue.Empty:
            pass
    os.system("tmux copy-mode -q -t 1.top")
    sleep(user_sleeptime)

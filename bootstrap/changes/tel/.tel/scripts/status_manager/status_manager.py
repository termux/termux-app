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
term = Terminal(force_styling=False) #force required if output not a tty
homedir = os.path.expanduser("~")
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
            new_output = ("Loading.. " + status_scripts[n].split("/status/",1)[1]) # show 'Loading.. filename' (remove path from name)
            draw_status_bar(status_bar, n, new_output)
            status_bar.append(new_output)
    return status_bar


def draw_status_bar(prev_status_bar,script_num,new_output):
    status_bar = prev_status_bar
    for line in range(0,len(prev_status_bar)):
        if line == script_num:
            if new_output != prev_status_bar[line]:
                status_bar[line] = new_output
                if len(str(term.strip_seqs(new_output))) > term.width:
                    new_output = new_output[:term.width -8]
                    new_output = new_output + term.normal + '..'
                    os.system("tel-status " + str(line) + " " + str(term.strip_seqs(new_output)))
                else:
                    os.system("tel-status " + str(line) + " " + str(term.strip_seqs(new_output)))
    return status_bar

#read output from status script wrappers stdout, when line has been read, puts it into queue
def read_output(pipe, q):
    while True:
        l = pipe.readline()
        q.put(l)


def main_loop():
    # get status scripts and init status bar
    status_scripts = get_scripts()
    loading_status_bar = init_status_bar(status_scripts)
    status_bar = loading_status_bar
    all_procs = []
    proc_queues = []
    proc_threads = []
    
    for script_num in range(0,len(status_scripts)):
    # start all script as subprocess
        new_process = subprocess.Popen(["./status_script_wrapper.sh", status_scripts[script_num], str(term.width)], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        all_procs.append(new_process)
    # create queue for storing script output
        create_queue = queue.Queue()
        proc_queues.append(create_queue)
    # create thread to run subprocess async
        create_thread = threading.Thread(target=read_output, args=(all_procs[script_num].stdout, proc_queues[script_num]))
        proc_threads.append(create_thread)
    # start thread to read output from subprocess
        proc_threads[script_num].daemon = True
        proc_threads[script_num].start()
    
    last_measured = term.height, term.width #initial measurement required to pass to func

    while True:
        new_output = None
        prev_status_bar = status_bar
        
        for process in range(0,len(all_procs)):
            all_procs[process].poll()
        # check if any subprocess has finished
        
        for process in range(0,len(all_procs)):
            if all_procs[process].returncode is not None:
            #    print(str(all_procs[process].returncode))
                break
        for process in range(0,len(proc_queues)):
            try:
                new_output = None
                new_output = proc_queues[process].get(False)
                #if new_output:
                new_output = new_output.decode('utf-8').rstrip('\r|\n')
                prev_status_bar = draw_status_bar(prev_status_bar, process, new_output)
            except queue.Empty:
                pass
        sleep(user_sleeptime)
# end main_loop

def clean_up():
#    for n in range(0,len(status_scripts)):
#       os.system('tel-delete-status ' + str(n))
    os.system('tel-delete-status -1')

if __name__ == "__main__":
    try:
        main_loop()
    except Exception as e:
        print(e.message, e.args)
    finally:
        clean_up()
        exit()

#!/usr/bin/env python
#tel notification status script
#v0.5 sealyj 17/10/20
import os
import json
import blessed
import subprocess
from blessed import Terminal
term = Terminal(force_styling=True) #force required if output not a tty
notification_display_type = os.environ['NOTIFICATIONS_DISPLAY_TYPE']
notificationsicon = " "
notification_list = []
homedir = os.path.expanduser("~")
try:
    notification_list = []
    if notification_display_type == "scroll":
        file_name = homedir + "/.tel/data/notifications"
        with open(file_name,"r+") as f_in:
            for line in f_in:
                if len(line.split()) == 0:
                    continue
                else: #only append non-empty lines
                    notification_list.append(line)
            if len(notification_list) > 1:
                #scroll one forward each time script is ran
                print(notificationsicon + notification_list[0].strip())
                f_in.seek(0)
                for eachline in notification_list[1:]:
                    f_in.write(eachline + "\n")
                f_in.write(notification_list[0] + "\n")
            elif len(notification_list) < 1:
                print(notificationsicon + "no new notifications")
            elif len(notification_list) == 1:
                print(notificationsicon + notification_list[0])
    else:
        latest = subprocess.check_output(["tail", "-n1", homedir + "/.tel/data/notifications"],universal_newlines=True, timeout=5) #strip()
        if latest:
            print(notificationsicon + latest)
        else:
            print(notificationsicon + " No new notifications")
except:
    print('except')

#!/usr/bin/env python
# Script to format and display android notifications from the termux api.
# version 0.12 - SealyJ
# required pip pkgs: blessed
import json
import string
import os
import subprocess
import sys
import re
import time
from blessed import Terminal
term = Terminal(force_styling=True) #force required if output not a tty
homedir = os.path.expanduser("~")
#user config
ignored_strings = []
ignored_strings_file = homedir + "/.tel/configs/notifications/ignored_strings"  
with open(ignored_strings_file) as igstrings:
    ignored_strings = igstrings.read().splitlines()
print(ignored_strings)

ignored_pkgs = []
ignored_pkgs_file = homedir + "/.tel/configs/notifications/ignored_pkgs"
with open(ignored_pkgs_file) as igpkg:
    ignored_pkgs = str(igpkg.read().splitlines())
color_output = True

#import env vars
refresh = int(os.environ['NOTIFICATIONS_CHECK_DELAY'])
 #number of seconds between new notifs check
color_output = os.environ['COLOR_APP_TITLES']
if color_output == 'true':
    color_output = True
else:
    color_output = False
notifications_enabled = os.environ['NOTIFICATIONS_ENABLED']
if notifications_enabled != 'true':
    print("Notifications are disabled in user config, exiting..")
    exit()

#output path
text_path = homedir + "/.tel/data/notifications"

debug_mode = False
if debug_mode is True:
    print(""" !!!!!       DEBUG MODE IS ENABLED    !!!!!  """)

outputList = []

#define color for each package
#see https://blessed.readthedocs.io/en/latest/colors.html for color definitions
# or use the color picker tool in ~/.tel/scripts

def pickColor(num,notifications):
    if notifications[num]["packageName"] == "com.twitter.android":
        return term.aqua
    elif notifications[num]["packageName"] == "com.android.systemui":
        return term.orange
    elif notifications[num]["packageName"] == "org.thoughtcrime.securesms":
        return term.blue
    elif notifications[num]["packageName"] == "com.android.dialer":
        return term.green
    elif notifications[num]["packageName"] == "com.instagram.android":
        return term.darkmagenta
    elif notifications[num]["packageName"] == "org.telegram.messenger":
        return term.turquoise2
    elif notifications[num]["packageName"] == "com.spotify.music":
        return term.springgreen
    elif notifications[num]["packageName"] == "com.whatsapp":
        return term.limegreen
    elif notifications[num]["packageName"] == "com.snapchat.android":
        return term.yellow2
    elif notifications[num]["packageName"] == "com.google.android.gm":
        return term.red2
    elif notifications[num]["packageName"] == "ch.gridvision.ppam.androidautomagic":
        return term.purple
    elif notifications[num]["packageName"] == "com.google.android.youtube":
        return term.red
    elif notifications[num]["packageName"] == "org.schabi.newpipe":
        return term.red
    elif notifications[num]["packageName"] == "in.dc297.mqttclpro":
        return term.mediumorchid4
    elif notifications[num]["packageName"] == "com.paypal.android.p2pmobile":
        return term.blue

    else:
        reset = term.normal
        return reset
def text_wrapper(inputList):
    cleanedinputList = inputList
    outputList.clear()
    for li in range(0,len(cleanedinputList)):
        outputList.append(cleanedinputList[li])
    #write to file
    if outputList:
        with open(text_path, 'w') as f:
            f.truncate(0)
            for item in outputList:
                #item = term.wrap(item)
                f.write("%s\n"%item)
        return outputList

def printnotifications(newNotifications):
    formattedList = []
    formattedList = text_wrapper(newNotifications) #writes to file

def getnotifications(oldNotifications):
    global notifications
    output_list = [] #colored and formatted string list 
    output_list.clear()
    newNotifs = [] 
    notifications = [] #raw json /  nested list
    #get notifs from termux api
    try:
        os.system("pkill -f 'NotificationList'")
        termuxapi = subprocess.run("termux-notification-list", stdout=subprocess.PIPE, universal_newlines=True, timeout=15)
    except:
        return None
    #convert stdoutput (json) to list (containing dicts for each notif)
    try:
        notifications = json.loads(termuxapi.stdout)
        print("Success decode json")
    except:
        print("Failed to decode json")
    #nothing is regarded as new because we started another loop
    newNotifs = []
    sanitised_notifs = []
    #find notifs from desired packages
    for n in range(0,len(notifications)):
        if notifications[n]["packageName"] not in ignored_pkgs and notifications[n] not in oldNotifications:
            # if notif hasnt already been seen in previous loop add to new list
            if notifications[n] and notifications[n]["title"] is not [None, "", " ", "  "] and notifications[n]["content"] is not [None, "", " ", "  "]:
                ignore_string_switch = False
                for item in range(0,len(ignored_strings)):
                    if ignore_string_switch == False and (notifications[n]["title"].find(ignored_strings[item]) != -1 or notifications[n]["content"].find(ignored_strings[item]) != -1):
                        ignore_string_switch = True
                        break
                        #compared for each item
                if ignore_string_switch == False and notifications[n] not in newNotifs:
                    newNotifs += [notifications[n]]
    oldNotifications = notifications
    if newNotifs:
        for notif in range(0,len(newNotifs)):
            # feel free to try to fix the crazy strings telegram sometimes puts into notifications in a cleaner way, this works for now 
            tit = newNotifs[notif]["title"].strip()
            cont = newNotifs[notif]["content"].strip()
            cont2 = cont.replace(u'\u200f', '')
            cont3 = cont2.replace(u'\u200e', '')
            tit2 = tit.replace(u'\u200e', '')
            tit3 = tit2.replace(u'\u200f', '')
            title = tit3.strip()
            content = cont3.strip()
            if color_output is True:
                col = pickColor(notif,newNotifs) 
                title = col + title + term.normal + ": "  
                output = title + content
                output = output + "\n"
            else:
                output = title + content + "\n"
            if title and content:
                output_list.append(output)
        return output_list
    else:
        return None

if __name__ == "__main__":
    try:
        oldNotifications = []
        while True:
            newNotifications = getnotifications(oldNotifications)
            if newNotifications:
                printnotifications(newNotifications)
            time.sleep(refresh)
            oldNotifications = notifications
    except KeyboardInterrupt:
        print('User exited Notification script with ctrl + c')
    finally:
        print("Exiting notification script")

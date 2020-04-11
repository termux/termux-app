#!/data/data/com.termux/files/usr/bin/python3.8
# Script to format and display android notifications from termux api.
# run with ./.notifications & disown
# Written by SealyJ
# github.com/sealedjoy
# Telegram @SealyJ
# version 0.11
# required pip pkgs: colored
import json
import os
import subprocess
import sys
import re
import time
import colored

# Todo: 
# ignored notification list partial string matching for telegram new messages etc
# read env vars
#user config
refresh = 3 #number of seconds between new notifs check
color_output = False # disabled until supported by helpers.sh 
ignoredNotifications = ["‎‏", "Telegram: Updating"] #string as part of list will ignore a specific static string notification 

#setup output path
homedir = os.path.expanduser("~")
text_path = homedir + "/.tel/data/notifications"

debug_mode = False
if debug_mode is True:
    print(""" !!!!!       DEBUG MODE IS ENABLED    !!!!!  """)

outputList = []
oldNotifs = []

# list of package names to display notifications from
pkgs = ("com.twitter.android", "com.instagram.android", "org.telegram.messenger", "com.spotify.music", "com.whatsapp", "com.snapchat.android", "ch.gridvision.ppam.androidautomagic")

#define color for each package
def pickColor(num,notifications):
    if notifications[num]["packageName"] == "com.twitter.android":
        return colored.fg(5)
    elif notifications[num]["packageName"] == "com.instagram.android":
        return colored.fg("magenta")
    elif notifications[num]["packageName"] == "org.telegram.messenger":
        return colored.fg("blue")
    elif notifications[num]["packageName"] == "com.spotify.music":
        return colored.fg("green")
    elif notifications[num]["packageName"] == "com.whatsapp":
        return colored.fg("green")
    elif notifications[num]["packageName"] == "com.snapchat.android":
        return colored.fg("yellow")
    else:
        reset = colored.attr('reset')
        return reset


def remove_tags(text):
    newtext = ''.join(ET.fromstring(text).itertext())
    return newtext

def cleanhtml(raw_html):
    cleanr = re.compile('<.*?>')
    cleantext = re.sub(cleanr, '', raw_html)
    return cleantext

def remove_formatting(inputList):
    outputList.clear()
    for notif in inputList:
      #  clean = cleanhtml(notif)
        clean = remove_tags(notif)
        outputList.append(clean)
    return outputList

def text_wrapper(inputList):
    #currently not used for wrapping just prints to file

    #width = os.get_terminal_size().columns
   # if debug_mode is True:
  #      print("Terminal Width is : " + str(width))
    #width = width - 4
    cleanedinputList = inputList
 #   cleanedinputList = remove_formatting(inputList)
    outputList.clear()
    for li in range(0,len(cleanedinputList)):
       # clean_line = cleanhtml(inputList[li])
        outputList.append(cleanedinputList[li])
    #write to file
    if outputList:
        with open(text_path, 'w') as f:
            f.truncate(0)
            for item in outputList:
                        f.write("%s\n" % item)
        return outputList

def printnotifications(newNotifications):
    formattedList = []
    formattedList = text_wrapper(newNotifications) #writes to file
    #print(newNotifications)
 

def getnotifications(oldNotifications):
    global notifications
    output_list = [] #colored and formatted string list 
    output_list.clear()
    newNotifs = [] 
    notifications = [] #raw json /  nested list
    #get notifs from termux api
    try:
        termuxapi = subprocess.run("termux-notification-list", stdout=subprocess.PIPE, universal_newlines=True, timeout=25)
    except:
        return None
    #convert stdoutput (json) to list (containing dicts for each notif)
    notifications = json.loads(termuxapi.stdout)
    #nothing is regarded as new because we started another loop
    newNotifs = []

    #find notifs from desired packages
    for n in range(0,len(notifications)):
        if notifications[n]["packageName"] in pkgs and notifications[n]["content"] not in ignoredNotifications:
            # if notif hasnt already been seen in previous loop add to new list
             if notifications[n] not in oldNotifications and notifications[n]["title"] is not [None, "", " "] and notifications[n]["content"] is not [None, "", " "]:
                newNotifs += [notifications[n]]
                if debug_mode is True:
                    print(notifications[n])
    oldNotifications = notifications
        #print any new notifs together
    if newNotifs:
        for notif in range(0,len(newNotifs)):
            if debug_mode is True:
                content = newNotifs[notif]["content"] + " (debug) KEY: " + newNotifs[notif]["key"]
            else:
                content = newNotifs[notif]["content"]

            if color_output is True:
                col=pickColor(notif,newNotifs) 
                col_title =  col + " " + newNotifs[notif]["title"] + ": " + colored.attr('reset')
                col_title_fixed = col_title.replace("<200e>", "")
                output = col_title_fixed + content
            else:
                title = newNotifs[notif]["title"] + ": "
                output = title + content
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

    #except:
    #    print(colored.fg(1), "!!!---> Exception occured in notification script please report to the TEL telegram group <---¡¡¡", colored.attr("reset"))
    finally:
        print("Exiting notification script")

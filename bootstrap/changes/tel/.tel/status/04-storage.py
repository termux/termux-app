#!/usr/bin/env python
#tel network status script
#v0.5 27/08/20
#from time import sleep
import os
import shutil
import json
import blessed
import subprocess
from blessed import Terminal
term = Terminal(force_styling=True) #force required if output not a tty
sd_card_path = os.environ['PATH_TO_SD']

def get_size(bytes, suffix="B"):
    """
    Scale bytes to its proper format
    e.g:
        1253656 => '1.20MB'
        1253656678 => '1.17GB'
    """
    factor = 1024
    for unit in ["", "K", "M", "G", "T", "P"]:
        if bytes < factor:
            return f"{bytes:.2f}{unit}{suffix}"
        bytes /= factor

def size_as_percentage(total,free):
    perc = total/100
    perc_free = int(free / perc)
    return(perc_free)

def pick_color(perc_free):
    if perc_free > 35:
        col = term.green
    elif perc_free > 25 and perc_free <= 35:
        col = term.yellow
    elif perc_free > 15 and perc_free <= 25:
        col = term.orange
    elif perc_free >= 0 and perc_free <= 15:
        col = term.red
    return col

# get IO statistics since boot
homedir = os.path.expanduser("~")

try:
    internal = shutil.disk_usage("/storage/emulated/0")
    percentage_free = size_as_percentage(internal.total,internal.free)
    col = pick_color(percentage_free)

    if sd_card_path != "none":
        external = shutil.disk_usage(sd_card_path)
        percentage_free_ext = size_as_percentage(external.total,external.free)
        col_ext = pick_color(percentage_free_ext)

        print(col + " " + term.normal + str(get_size(internal.free)) + " available " + str(get_size(external.free)) + col_ext + "ﳚ " + term.normal)
    else:
        print(col + " " + term.normal + str(get_size(internal.free)) + " available @ '~/storage'")
except:
    print(' storage error, check permissions')

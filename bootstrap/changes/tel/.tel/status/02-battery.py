#!/usr/bin/env python
#v0.4 24/04/20
import os
import json
import blessed
import subprocess
from blessed import Terminal
term = Terminal(force_styling=True) #force required if output not a tty

chargeicon=""
dischargeicon=""
#THERMICON=$(echo $'\ufa0e')
#HEALTHICON=$(echo $'\uf7df')    
homedir = os.path.expanduser("~")
battery_file = homedir + "/.tel/data/.batt"
os.system("pkill -f 'BatteryStatus'")
try:
    battery = json.loads(subprocess.check_output(["termux-battery-status"], universal_newlines=True, timeout=5)) 

    if battery['current'] < 0: #remove negative symbol
        current = -(int(battery["current"]))
    else:
        current = int(battery["current"])
    if current > 10000: # or battery["current"] < -10000:
        # device reporting in microamps
        current = int(current / 1000)
    else:
        # else device likely already in milliamps
        pass

    #print(battery)
    if battery["status"] == "DISCHARGING":
        status = "Discharging"
        if battery["percentage"] >= 0 and battery["percentage"] <= 20:
            strengthcol = term.red
        elif battery["percentage"] > 20 and battery["percentage"] < 40:
            strengthcol = term.orange
        elif battery["percentage"] > 40 and battery["percentage"] < 50:
            strengthcol = term.yellow
        else:
            strengthcol = term.green
        battery_str = strengthcol + dischargeicon + term.normal + " " + str(battery["percentage"]) + "% " + status + " @ " +  str(current) + "mA"
        print(battery_str)
    else: #if charging
        if current >= 0 and current <= 200:
            strengthcol = term.red
            status = "Trickle Charging"
        elif current > 200 and current < 500:
            strengthcol = term.orange
            status = "Slow Charging"
        elif current > 500 and current < 1000:
            strengthcol = term.yellow
            status = "Charging"
        elif current > 1000 and current < 1670:
            strengthcol = term.greenyellow
            status = "Quick Charging"
        else:
            strengthcol = term.green
            status = "Fast Charging"
       #status = Charging
       # strengthcol = term.greenyellow
        battery_str = strengthcol + chargeicon + term.normal + " " + str(battery["percentage"]) + "% " + status + " @ " +  "+" + str(current) + "mA"
        if battery["health"] != "GOOD":
            warning = term.red + "  " + term.normal
            print(battery_str + warning)
        else:
            with open(battery_file, 'w+') as out_file:
                out_file.write(battery_str + "\n")
            print(battery_str)
except:
    #print(dischargeicon + " loading data")
    with open(battery_file, 'r+') as in_file:
        print(in_file.read())
exit()

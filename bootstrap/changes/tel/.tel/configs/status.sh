#!/data/data/com.termux/files/usr/bin/bash

# TEL config is stored in this file.


# STATUS WINDOW
export STATUS_ENABLED=true #status pane on startup?
export STATUS_CENTER=true #center output in status pane [true/false]
export STATUS_SPACING_CHAR=" " #char to create spacing if center is enabled, try "="
export STATUS_RELOAD=2 #time to sleep betweem status pane refresh (higher may save battery) [seconds]
export STATUS_TIMEOUT=4 #timeout for status scripts (if status not fully loading try increasing) [seconds]
export STATUS_COLOR=false #enable status color via lolcat [true/false]
export STATUS_COLOR_SPREAD="150" #lolcat color spread value
export NOTIFICATIONS=true #enable notification display in status


# USER PREFS
export EDITOR=nano #used for todo -e command


#!/usr/bin/env bash

############## TEL User Preferences Configuration #########
#               Behavior and program preferences          #
#                   Restart to see changes             	  #
###########################################################

# Basic #
export NAME='User' #main/home window name
export EDITOR=nano # set your favourite file editor: [nano,vim,neovim,sublime,emacs..]
export LOCATION=London #weather command and weather status script - [string]
export WEATHER_CHECK=60 # weather status script, check the weather forcast [minutes]
export PATH_TO_SD=none # shows space in status, if not using an sdcard must be set to: none, else path must be absolute (no '~' or '$HOME' !) [none / pathtosdcard]

# Startup #
export STARTUP_ANIMATION_ENABLED=true  #show the animation on startup? user can also press alt + q, or ctrl + c to skip [true/false]
export MOTD_HINTS=false #swap motd for a version that supplies a tel usage hint each startup, false restores old user motd [true/false]
export SSH_SERVER=false #this needs to be configured before being set to true, search for termux openssh. If you don't know what this is, leave as 'false'! It has BIG security implications and is only recommended for advanced users who use ssh daily. [true/false]

# Powersaver # - see status.sh for further tweaks to save battery
export POWER_SAVER_ACTIVE=true #prevents various processes using battery whilst tel is inactive [true/false]
export POWER_SAVER_TIMEOUT=57 #seconds of inactivity required to start power saver, ideally set to your screen timeout minus 3 seconds [seconds]
export POWER_SAVER_DISPLAY_COMMAND='' #examples: "uptime, neofetch --stdout"

#!/usr/bin/env bash

############## TEL User Preferences Configuration #########
#               Behavior and program preferences          #
#                   Restart to see changes             	  #
###########################################################

export EDITOR=nano # set your favourite editor: [nano,vi,vim,neovim,sublime,emacs etc] if unsure leave as nano.
export LOCATION=London #used for weather command and weather status script
export WEATHER_CHECK=60 # weather status script, check the weather forcast [minutes]
export STARTUP_ANIMATION_ENABLED=true  #show the animation on startup? user can also press alt + q to skip [true/false] 
export POWER_SAVER_ACTIVE=true #prevents various processes using battery whilst tel is inactive [true/false]
export POWER_SAVER_TIMEOUT=59 #seconds of inactivity required to start power saver, ideally set to your screen timeout minus 1 second [seconds]
export POWER_SAVER_DISPLAY_COMMAND='' #examples: "uptime, neofetch --stdout"
export NAME='User' #used when TEL refers to you ( statusbar startsup)
export PATH_TO_SD=none #if not using an sdcard must be set to: none, else path must be absolute(no ~ or $HOME) [none / pathtosdcard]
export SSH_SERVER=false #this needs to be configured before being set to true, search for termux openssh. If you don't know what this is, leave as 'false'! It has security implications and is only recommended for advanced users who use ssh daily. [true/false]
export REBUILD_CACHES_STARTUP=false #keeps contacts and app lists up to date, disable for faster startup. but user will have to update manually. (using app -u, phone -u) [true/false]

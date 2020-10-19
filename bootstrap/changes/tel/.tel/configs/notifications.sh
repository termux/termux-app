#!/usr/bin/env bash

############### TEL Notifications Configuration ###########
#    Status module at ~/.tel/status/$$-notifications.sh   #
#                   Restart to see changes             	  #
###########################################################

# Enable / Disable Notifications completely
export NOTIFICATIONS_ENABLED=true #enable notification daemon (requires api permissions) [true/false]
export COLOR_APP_TITLES=true  #apply color to package titles in notification display [true/false]
export NOTIFICATIONS_CHECK_DELAY=2  #Check for new notifs every [seconds]
export NOTIFICATIONS_DISPLAY_TYPE=scroll # in the status bar module, scroll through each notification or just show latest [scroll/latest]

### NOTE - EXTRA OPTIONS AVAILIBLE IN ~/.tel/configs/notifications/

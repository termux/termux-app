#!/usr/bin/env bash

############### TEL Status Window Configuration ###########
#       Status scripts located at ~/.tel/status/*         #
#              use tel-restart to see changes             #
###########################################################

# Enable / Disable Status window completely
export STATUS_WINDOW_ENABLED=true #[true/false]
#UNSUPPORTED export STATUS_CENTER=true #center output in status pane [true/false]
#export STATUS_RELOAD=1 #time to sleep between status pane refresh (higher may save battery) [seconds]

export STATUS_MANAGER_SLEEP=1 #time to sleep each status screen between each cycle, lower = more responsive, higher = more battery saved [seconds]
export STATUS_SCRIPTS_SLEEP=6
#export STATUS_API_PAUSE=3 #time to wait between status pane refresh (higher values will save battery at expense of more regular refresh) [seconds]
export STATUS_TIMEOUT=45 #timeout for misbehaviour in status scripts (if status not fully loading try increasing) [seconds]


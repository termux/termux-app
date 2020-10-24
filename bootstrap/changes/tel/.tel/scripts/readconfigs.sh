#!/data/data/com.termux/files/usr/bin/bash
# Handles TEL's configs 

# Version Info
#. ~/.tel/.version
#. ~/.tel/scripts/status_manager/.version

# Config files - these each source relevant user configs in ~/storage/shared/tel
source <(cat ~/.tel/configs/*.sh)

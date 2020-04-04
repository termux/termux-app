#!/system/bin/sh
# version 0.1
# Created by SealyJ aka Guiseppe for TEL - March 2020 
# Tested on android 9, lineage OS
# Contact: github.com/sealedjoy, Telegram @SealyJ

######## USER CONFIG #######
CHARGEICON=
DISCHARGEICON=
#THERMICON=$(echo $'\ufa0e')
#HEALTHICON=$(echo $'\uf7df')
MAXLINELEN=70

RED='\033[0;31m'
NC='\033[0m' # No Col

##### GET BATT DATA ######
BATCAP=$(cat /sys/class/power_supply/battery/capacity)
BATCAP="$BATCAP"
BATSTATUS=$(cat /sys/class/power_supply/battery/status)
#TEMP=$(cat /sys/class/power_supply/battery/temp)
#TEMP=$(($TEMP / 10))
#HEALTH=$(cat /sys/class/power_supply/battery/health)
BATT_CURRENT=$(cat /sys/class/power_supply/battery/current_now)
BATT_CURRENT=$(($BATT_CURRENT / 1000))


###### CHARGING? #######
if [ $BATSTATUS == 'Charging' ]
   then
    CHARGESPEED=$(cat /sys/class/power_supply/battery/charge_type) 
    CHARGESPEED="$CHARGESPEED "
    SELECTEDBATTICON=$CHARGEICON
    COL1=$NC
elif [ $BATSTATUS == 'Discharging' ]
  then
  CHARGESPEED=""
  SELECTEDBATTICON=$DISCHARGEICON
  if [ $BATCAP -lt 25 ] ; then
    COL1=$RED
  else
    COL1=$NC
  fi
fi

## UPDATE ICON TO REFLECT STATE ##
BATTICON=$SELECTEDBATTICON


##### BEGIN OUTPUT #####
BATT="$BATCAP% $CHARGESPEED$BATSTATUS @ $BATT_CURRENT mA/h " 
#optionally addable stats [ $HEALTH $TEMP°C ]

echo -e "        ${COL1}$BATTICON${NC} ${BATT:0:$MAXLINELEN}"
exit 0



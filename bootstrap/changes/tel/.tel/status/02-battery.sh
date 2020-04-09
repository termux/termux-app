#!/system/bin/sh
# version 0.1

######## USER CONFIG #######
CHARGEICON=
DISCHARGEICON=
#THERMICON=$(echo $'\ufa0e')
#HEALTHICON=$(echo $'\uf7df')

RED='\033[0;31m'
NC='\033[0m' # No Col

batteryapi=$(termux-battery-status)

##### GET BATT DATA ######
capacity=$(echo "$batteryapi" | jq -r .percentage)
status=$(echo "$batteryapi" | jq -r .status)
#temp=$(echo "$batteryapi" | jq -r .temperature)
#health=$(echo "$batteryapi" | jq -r .health)
current=$(echo "$batteryapi" | jq -r .current)
current=$(($current / 1000))


###### CHARGING? #######
if [ $status == 'CHARGING' ]
   then
   # CHARGESPEED=$(cat /sys/class/power_supply/battery/charge_type) 
   # CHARGESPEED="$CHARGESPEED "
    $status="Charging"
    SELECTEDBATTICON=$CHARGEICON
    COL1=$NC
elif [ $status == 'DISCHARGING' ]
  then
  $status="Discharging"
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
BATT="$capacity% $status @ $current mA/h " 
#optionally addable stats [ $HEALTH $TEMP°C ]

echo -e "        ${COL1}$BATTICON${NC} ${BATT}"
exit 0



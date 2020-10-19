#!/data/data/com.termux/files/usr/bin/bash
icon=""

# CPU info
num_cores=$(grep -c ^processor /proc/cpuinfo | tr -d '\n')
load=$(uptime  | grep -o '[0-9]\+\.[0-9]\+*' | head -n1 | tr -d '\n')
#cpu_load=`expr $load * 100`
cpu_load=$(echo $load 100 | awk '{printf "%4.3f\n",$1*$2}')
cpu_int=$(printf '%.0f' "$cpu_load")
percpu=$((cpu_int / num_cores))


# RAM info
MEMINFO=$(</proc/meminfo)

MEMINFO_MEMFREE=$(echo "${MEMINFO}" | awk '$1 == "MemFree:" {print $2 * 1024}')
MEMINFO_MEMTOTAL=$(echo "${MEMINFO}" | awk '$1 == "MemTotal:" {print $2 * 1024}')
MEMINFO_FILE=$(echo "${MEMINFO}" | awk '{MEMINFO[$1]=$2} END {print (MEMINFO["Active(file):"] + MEMINFO["Inactive(file):"]) * 1024}')
MEMINFO_SRECLAIMABLE=$(echo "${MEMINFO}" | awk '$1 == "SReclaimable:" {print $2 * 1024}')

MEMINFO_MEMAVAILABLE=$((
  MEMINFO_MEMFREE - LOW_WATERMARK
  + MEMINFO_FILE - ((MEMINFO_FILE/2) < LOW_WATERMARK ? (MEMINFO_FILE/2) : LOW_WATERMARK)
  + MEMINFO_SRECLAIMABLE - ((MEMINFO_SRECLAIMABLE/2) < LOW_WATERMARK ? (MEMINFO_SRECLAIMABLE/2) : LOW_WATERMARK)
))

if [[ "${MEMINFO_MEMAVAILABLE}" -le 0 ]]
then
  MEMINFO_MEMAVAILABLE=0
fi
free_memory=$(numfmt --to iec --format "%8.1f" $MEMINFO_MEMAVAILABLE)
total_memory=$(numfmt --to iec --format "%8.1f" $MEMINFO_MEMTOTAL)


printf "%s" "${icon} ${num_cores} Cores @ ${percpu}% avgload Mem: $free_memory / $total_memory availible"

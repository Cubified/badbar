#!/bin/sh

# This is the expanded form of the single line script in config.def.h
# This file serves no purpose other than clarity

# Heavily inspired by https://github.com/moisespsena/linux-cpu-usage

stat=`cat /proc/stat | grep '^cpu '`
idle=`cat /proc/stat | awk '/^cpu / {printf("%f", $5)}'`
total=0

for val in $stat; do
  if [ ! "$val" = "cpu" ]; then
    total=`echo "$total+$val" | bc`
  fi
done

if [ ! -e /tmp/.cpu_usage ]; then
  mkdir /tmp/.cpu_usage
  echo "0" > /tmp/.cpu_usage/prev_idle
  echo "0" > /tmp/.cpu_usage/prev_total
fi

prev_idle=`cat /tmp/.cpu_usage/prev_idle`
prev_total=`cat /tmp/.cpu_usage/prev_total`

diff_idle=`echo "$idle-$prev_idle" | bc`
diff_total=`echo "scale=5; $total-$prev_total" | bc`
diff_diffs=`echo "$diff_total-$diff_idle" | bc`
diff_ratio=`echo "scale=5; $diff_diffs/$diff_total" | bc`
diff_percent=`echo "1000*$diff_ratio" | bc`
diff_percent_plus5=`echo "$diff_percent+5" | bc`
diff_usage=`echo "$diff_percent_plus5 10" | awk '{printf "%.0f", $1/$2}'` # awk for formatting

echo "$idle" > /tmp/.cpu_usage/prev_idle
echo "$total" > /tmp/.cpu_usage/prev_total

echo $diff_usage

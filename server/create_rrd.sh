#!/usr/bin/env bash

NAME="ups"    # this is a name of the device configured in the controller !
RRDDIR="/var/db/rrd/"
STEP=60
HEARTBEAT=$((2*$STEP))
START=$(date +%s)

if [ ! -d "$RRDDIR" ]; then
    mkdir -p "$RRDDIR" || exit
fi

RRDf="$RRDDIR$NAME.rrd"

if [ -f "$RRDf" ]; then
    echo "$RRDf" exists!
    exit
fi

rrdtool create "$RRDf" \
            --start $START  --step $STEP \
            DS:line:GAUGE:$HEARTBEAT:U:U   \
            DS:power:GAUGE:$HEARTBEAT:U:U   \
            DS:temp:GAUGE:$HEARTBEAT:U:U   \
            DS:battstatus:GAUGE:$HEARTBEAT:U:U   \
            DS:battery:GAUGE:$HEARTBEAT:U:U   \
            RRA:AVERAGE:0.5:1:1440  \
            RRA:AVERAGE:0.5:7:1440  \
            RRA:AVERAGE:0.5:30:17280

if [ -f "$RRDf" -a -s "$RRDf" ]; then
    echo "$RRDf" created
fi


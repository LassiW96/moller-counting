#!/bin/sh

## Setup your custom database directory
## Use: source setup_ana_env.sh

export DB_DIR=$SBS_REPLAY/DB
export DATA_DIR=$HOME/sbs/data
export ANALYZER_CONFIGPATH=$SBS_REPLAY/replay
export OUT_DIR=$HOME/sbs/Rootfiles
export LOG_DIR=/$(hostname -s)/work1/logs

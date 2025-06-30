#!/bin/sh

## Setup your custom database directory
## Use: source setup_ana_env.sh

## New environment for moller counting
export MOLLER_REPLAY=$HOME/JLab-analyzers/moller-counting/MOLLER-replay

export DB_DIR=$MOLLER_REPLAY/DB
export DATA_DIR=$HOME/JLab-analyzers/moller-counting-test/data
export ANALYZER_CONFIGPATH=$MOLLER_REPLAY/replay
export OUT_DIR=$HOME/JLab-analyzers/moller-counting-test/output
export LOG_DIR=$HOME/JLab-analyzers/moller-counting-test/logs

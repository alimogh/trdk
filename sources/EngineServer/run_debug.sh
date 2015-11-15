#!/bin/bash

export LD_LIBRARY_PATH=./:/usr/local/lib/

ulimit -c unlimited

./RobotEngine debug etc/twd 2>&1

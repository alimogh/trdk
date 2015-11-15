#!/bin/bash

export LD_LIBRARY_PATH=./:/usr/local/lib/

ulimit -c unlimited

./RobotEngine debug etc/terminal 2>&1

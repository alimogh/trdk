#!/bin/bash

export LD_LIBRARY_PATH=./:/usr/local/lib/

ulimit -c unlimited

./RobotEngine_dbg standalone etc/twd 2>&1

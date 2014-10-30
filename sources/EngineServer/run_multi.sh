#!/bin/bash

export LD_LIBRARY_PATH=./

ulimit -c unlimited

./RobotEngine debug etc/multi 2>&1

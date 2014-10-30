#!/bin/bash

export LD_LIBRARY_PATH=./

ulimit -c unlimited

./RobotEngine debug etc/mono 2>&1

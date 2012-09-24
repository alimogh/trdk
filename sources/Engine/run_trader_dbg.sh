#!/bin/bash

export LD_LIBRARY_PATH=./Lib_dbg:./Sys

./trader_dbg 2> >(grep -v 'order execution on unknonw orderID')

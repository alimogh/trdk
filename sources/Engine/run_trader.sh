#!/bin/bash

export LD_LIBRARY_PATH=./Lib:./Sys

./trader 2> >(grep -v 'order execution on unknonw orderID')

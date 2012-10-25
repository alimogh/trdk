#!/bin/bash

conf="Debug"

cd ./Engine
make CONF=${conf} clean
make CONF=${conf}
cd ..

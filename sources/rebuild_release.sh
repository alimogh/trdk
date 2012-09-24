#!/bin/bash

conf="Release"

cd ./Engine
make CONF=${conf} clean
make CONF=${conf}
cd ..

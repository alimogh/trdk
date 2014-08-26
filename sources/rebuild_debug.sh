#!/bin/bash

conf="Debug"

cd ./EngineServer
make CONF=${conf} clean
make CONF=${conf}
cd ..

#!/bin/bash

conf="Release"

cd ./EngineServer
make CONF=${conf} clean
make CONF=${conf}
cd ..

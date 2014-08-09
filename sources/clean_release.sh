#!/bin/bash

conf="Release"

cd ./EngineServer
make CONF=${conf} clean
cd ..

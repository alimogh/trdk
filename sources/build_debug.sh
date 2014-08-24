#!/bin/bash

conf="Debug"

cd ./EngineServer
make CONF=${conf}
cd ..

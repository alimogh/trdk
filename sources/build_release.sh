#!/bin/bash

conf="Release"

cd ./EngineServer
make CONF=${conf}
cd ..

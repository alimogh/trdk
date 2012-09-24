#!/bin/bash

conf="Release"

cd ./Engine
make CONF=${conf} clean
cd ..

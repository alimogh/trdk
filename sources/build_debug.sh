#!/bin/bash

conf="Debug"

cd ./Engine
make CONF=${conf}
cd ..

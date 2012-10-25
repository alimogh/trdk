#!/bin/bash

conf="Release"

cd ./Engine
make CONF=${conf}
cd ..

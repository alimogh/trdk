#!/bin/bash

outputDir="../output/Linux/"
outputLibDir="../output/Linux/Lib_dbg/"
sysDir="../output/Linux/Sys/"
conf="Debug"

mkdir ${outputDir}
mkdir ${outputLibDir}
mkdir ${sysDir}

cp ./Engine/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./Engine/run_trader_dbg.sh ${outputDir}
cp ./Core/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./PyApi/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./Interaction/Fake/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./Interaction/Enyx/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./Interaction/Lightspeed/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./Gateway/Service/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}

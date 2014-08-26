#!/bin/bash

outputDir="../output/"
outputLibDir="../output/"
conf="Debug"

mkdir ${outputDir}
mkdir ${outputLibDir}

cp ./Engine/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Core/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Strategies/Test/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Interaction/OnixsFixConnector/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./EngineServer/dist/${conf}/GNU-Linux-x86/* ${outputDir}
cp ./EngineServer/run_dbg.sh ${outputDir}

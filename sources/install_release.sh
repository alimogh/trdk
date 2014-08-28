#!/bin/bash

outputDir="../output/"
outputLibDir="../output/"
conf="Release"

mkdir ${outputDir}
mkdir ${outputLibDir}

cp ./Engine/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Core/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Strategies/Test/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Strategies/FxMb/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Interaction/OnixsFixConnector/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./EngineServer/dist/${conf}/GNU-Linux-x86/* ${outputDir}
cp ./EngineServer/run.sh ${outputDir}

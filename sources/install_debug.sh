#!/bin/bash

outputDir="../output/"
outputLibDir="../output/"
conf="Debug"

mkdir ${outputDir}
mkdir ${outputLibDir}

cp ./Engine/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Core/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Strategies/FxMb/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Interaction/OnixsFixConnector/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./Interaction/OnixsHotspot/dist/${conf}/GNU-Linux-x86/* ${outputLibDir}
cp ./EngineServer/dist/${conf}/GNU-Linux-x86/* ${outputDir}
cp ./EngineServer/run_dbg.sh ${outputDir}
cp ./EngineServer/run_mono_dbg.sh ${outputDir}
cp ./EngineServer/run_multi_dbg.sh ${outputDir}

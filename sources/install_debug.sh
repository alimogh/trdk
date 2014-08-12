#!/bin/bash

outputDir="../output/"
conf="Debug"

mkdir ${outputDir}

cp ./Engine/dist/${conf}/GNU-Linux-x86/* ${outputDir}
cp ./Core/dist/${conf}/GNU-Linux-x86/* ${outputDir}
cp ./Strategies/Test/dist/${conf}/GNU-Linux-x86/* ${outputDir}
cp ./Interaction/OnixsFixConnector/dist/${conf}/GNU-Linux-x86/* ${outputDir}
cp ./EngineServer/dist/${conf}/GNU-Linux-x86/* ${outputDir}

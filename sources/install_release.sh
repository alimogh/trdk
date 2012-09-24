#!/bin/bash

outputDir="../output/Linux/"
conf="Release"

mkdir ${outputDir}

cp ./Engine/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./Core/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./PyApi/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./Interaction/Fake/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./Interaction/Enyx/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./Interaction/Lightspeed/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./Gateway/Service/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}

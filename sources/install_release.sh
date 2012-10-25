#!/bin/bash

outputDir="../output/Linux/"
outputLibDir="../output/Linux/Lib/"
sysDir="../output/Linux/Sys/"
conf="Release"

mkdir ${outputDir}
mkdir ${outputLibDir}
mkdir ${sysDir}

cp /usr/local/boost/boost_1_51/lib/libboost_date_time.so.1.51.0 ${sysDir}
cp /usr/local/boost/boost_1_51/lib/libboost_thread.so.1.51.0 ${sysDir}
cp /usr/local/boost/boost_1_51/lib/libboost_system.so.1.51.0 ${sysDir}
cp /usr/local/boost/boost_1_51/lib/libboost_filesystem.so.1.51.0 ${sysDir}
cp /usr/local/boost/boost_1_51/lib/libboost_chrono.so.1.51.0 ${sysDir}
cp /usr/local/boost/boost_1_51/lib/libboost_signals.so.1.51.0 ${sysDir}
cp /usr/local/boost/boost_1_51/lib/libboost_regex.so.1.51.0 ${sysDir}
cp /usr/local/lib64/libstdc++.so.6.0.17 ${sysDir}libstdc++.so.6

cp ./Engine/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputDir}
cp ./Engine/run_trader.sh ${outputDir}
cp ./Core/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./PyApi/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./Interaction/Fake/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}
cp ./Gateway/Service/dist/${conf}/GNU_4.7.1-Linux-x86/* ${outputLibDir}

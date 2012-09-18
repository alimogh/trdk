#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc-4.7.1
CCC=g++-4.7.1
CXX=g++-4.7.1
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU_4.7.1-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Util.o \
	${OBJECTDIR}/Dispatcher.o \
	${OBJECTDIR}/Ini.o \
	${OBJECTDIR}/_ext/2108356922/Assert.o \
	${OBJECTDIR}/Main.o \
	${OBJECTDIR}/Trading.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/boost/boost_1_51/lib ../Common/dist/Debug/GNU_4.7.1-Linux-x86/libcommon.a -Wl,-rpath,../Core/dist/Debug/GNU_4.7.1-Linux-x86 -L../Core/dist/Debug/GNU_4.7.1-Linux-x86 -lCore_dbg -lboost_date_time -lboost_thread -lboost_system -lboost_filesystem -lboost_chrono -ldl

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trader_dbg

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trader_dbg: ../Common/dist/Debug/GNU_4.7.1-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trader_dbg: ../Core/dist/Debug/GNU_4.7.1-Linux-x86/libCore_dbg.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trader_dbg: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trader_dbg ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/Util.o: Util.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/Util.o Util.cpp

${OBJECTDIR}/Dispatcher.o: Dispatcher.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/Dispatcher.o Dispatcher.cpp

${OBJECTDIR}/Ini.o: Ini.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/Ini.o Ini.cpp

${OBJECTDIR}/_ext/2108356922/Assert.o: ../Common/Assert.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/2108356922
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/2108356922/Assert.o ../Common/Assert.cpp

${OBJECTDIR}/Main.o: Main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/Main.o Main.cpp

${OBJECTDIR}/Trading.o: Trading.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/Trading.o Trading.cpp

# Subprojects
.build-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Debug
	cd ../Core && ${MAKE}  -f Makefile CONF=Debug
	cd ../Interaction/Lightspeed && ${MAKE}  -f Makefile CONF=Debug
	cd ../Interaction/Enyx && ${MAKE}  -f Makefile CONF=Debug
	cd ../PyApi && ${MAKE}  -f Makefile CONF=Debug
	cd ../Interaction/Fake && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trader_dbg

# Subprojects
.clean-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Core && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Interaction/Lightspeed && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Interaction/Enyx && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../PyApi && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Interaction/Fake && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

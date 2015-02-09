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
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
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
	${OBJECTDIR}/Context.o \
	${OBJECTDIR}/ContextBootstrap.o \
	${OBJECTDIR}/Dispatcher.o \
	${OBJECTDIR}/Ini.o \
	${OBJECTDIR}/SubscriberPtrWrapper.o \
	${OBJECTDIR}/SubscriptionsManager.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-fPIC -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
CXXFLAGS=-fPIC -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=../Common/dist/Debug/GNU-Linux-x86/libcommon_dbg.a -Wl,-rpath,../Core/dist/Debug/GNU-Linux-x86 -L../Core/dist/Debug/GNU-Linux-x86 -lCore_dbg -lboost_date_time -lboost_thread -lboost_system -lboost_filesystem -lboost_chrono -lboost_regex -ldl

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngine_dbg.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngine_dbg.${CND_DLIB_EXT}: ../Common/dist/Debug/GNU-Linux-x86/libcommon_dbg.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngine_dbg.${CND_DLIB_EXT}: ../Core/dist/Debug/GNU-Linux-x86/libCore_dbg.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngine_dbg.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngine_dbg.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -ltcmalloc -shared -fPIC

${OBJECTDIR}/Context.o: Context.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Context.o Context.cpp

${OBJECTDIR}/ContextBootstrap.o: ContextBootstrap.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ContextBootstrap.o ContextBootstrap.cpp

${OBJECTDIR}/Dispatcher.o: Dispatcher.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Dispatcher.o Dispatcher.cpp

${OBJECTDIR}/Ini.o: Ini.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Ini.o Ini.cpp

${OBJECTDIR}/SubscriberPtrWrapper.o: SubscriberPtrWrapper.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SubscriberPtrWrapper.o SubscriberPtrWrapper.cpp

${OBJECTDIR}/SubscriptionsManager.o: SubscriptionsManager.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_ENGINE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SubscriptionsManager.o SubscriptionsManager.cpp

# Subprojects
.build-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Debug
	cd ../Core && ${MAKE}  -f Makefile CONF=Debug
	cd ../Interaction/LogReplay && ${MAKE}  -f Makefile CONF=Debug
	cd ../Interaction/OnixsFixConnector && ${MAKE}  -f Makefile CONF=Debug
	cd ../Strategies/FxMb && ${MAKE}  -f Makefile CONF=Debug
	cd ../Interaction/OnixsHotspot && ${MAKE}  -f Makefile CONF=Debug
	cd ../Interaction/Fake && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngine_dbg.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Core && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Interaction/LogReplay && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Interaction/OnixsFixConnector && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Strategies/FxMb && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Interaction/OnixsHotspot && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../Interaction/Fake && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

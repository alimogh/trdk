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
	${OBJECTDIR}/Consumer.o \
	${OBJECTDIR}/Context.o \
	${OBJECTDIR}/Instrument.o \
	${OBJECTDIR}/Interactor.o \
	${OBJECTDIR}/Log.o \
	${OBJECTDIR}/MarketDataSource.o \
	${OBJECTDIR}/Module.o \
	${OBJECTDIR}/Observer.o \
	${OBJECTDIR}/Position.o \
	${OBJECTDIR}/PositionReporter.o \
	${OBJECTDIR}/Security.o \
	${OBJECTDIR}/Service.o \
	${OBJECTDIR}/Settings.o \
	${OBJECTDIR}/Strategy.o \
	${OBJECTDIR}/StrategyPositionState.o \
	${OBJECTDIR}/TradeSystem.o \
	${OBJECTDIR}/Types.o


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
LDLIBSOPTIONS=../Common/dist/Debug/GNU-Linux-x86/libcommon.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore_dbg.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore_dbg.${CND_DLIB_EXT}: ../Common/dist/Debug/GNU-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore_dbg.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore_dbg.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/Consumer.o: Consumer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Consumer.o Consumer.cpp

${OBJECTDIR}/Context.o: Context.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Context.o Context.cpp

${OBJECTDIR}/Instrument.o: Instrument.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Instrument.o Instrument.cpp

${OBJECTDIR}/Interactor.o: Interactor.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Interactor.o Interactor.cpp

${OBJECTDIR}/Log.o: Log.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Log.o Log.cpp

${OBJECTDIR}/MarketDataSource.o: MarketDataSource.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MarketDataSource.o MarketDataSource.cpp

${OBJECTDIR}/Module.o: Module.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Module.o Module.cpp

${OBJECTDIR}/Observer.o: Observer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Observer.o Observer.cpp

${OBJECTDIR}/Position.o: Position.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Position.o Position.cpp

${OBJECTDIR}/PositionReporter.o: PositionReporter.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PositionReporter.o PositionReporter.cpp

${OBJECTDIR}/Security.o: Security.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Security.o Security.cpp

${OBJECTDIR}/Service.o: Service.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Service.o Service.cpp

${OBJECTDIR}/Settings.o: Settings.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Settings.o Settings.cpp

${OBJECTDIR}/Strategy.o: Strategy.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Strategy.o Strategy.cpp

${OBJECTDIR}/StrategyPositionState.o: StrategyPositionState.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/StrategyPositionState.o StrategyPositionState.cpp

${OBJECTDIR}/TradeSystem.o: TradeSystem.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TradeSystem.o TradeSystem.cpp

${OBJECTDIR}/Types.o: Types.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_CORE -D_DEBUG -I.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Types.o Types.cpp

# Subprojects
.build-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore_dbg.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

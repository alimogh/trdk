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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Algo.o \
	${OBJECTDIR}/Settings.o \
	${OBJECTDIR}/MarketDataSource.o \
	${OBJECTDIR}/Module.o \
	${OBJECTDIR}/AlgoPositionState.o \
	${OBJECTDIR}/TradeSystem.o \
	${OBJECTDIR}/Log.o \
	${OBJECTDIR}/PositionReporter.o \
	${OBJECTDIR}/PositionBundle.o \
	${OBJECTDIR}/Position.o \
	${OBJECTDIR}/Security.o


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
LDLIBSOPTIONS=../Common/dist/Release/GNU_4.7.1-Linux-x86/libcommon.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore.${CND_DLIB_EXT}: ../Common/dist/Release/GNU_4.7.1-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -shared -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore.${CND_DLIB_EXT} -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/Algo.o: Algo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Algo.o Algo.cpp

${OBJECTDIR}/Settings.o: Settings.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Settings.o Settings.cpp

${OBJECTDIR}/MarketDataSource.o: MarketDataSource.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/MarketDataSource.o MarketDataSource.cpp

${OBJECTDIR}/Module.o: Module.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Module.o Module.cpp

${OBJECTDIR}/AlgoPositionState.o: AlgoPositionState.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/AlgoPositionState.o AlgoPositionState.cpp

${OBJECTDIR}/TradeSystem.o: TradeSystem.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/TradeSystem.o TradeSystem.cpp

${OBJECTDIR}/Log.o: Log.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Log.o Log.cpp

${OBJECTDIR}/PositionReporter.o: PositionReporter.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/PositionReporter.o PositionReporter.cpp

${OBJECTDIR}/PositionBundle.o: PositionBundle.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/PositionBundle.o PositionBundle.cpp

${OBJECTDIR}/Position.o: Position.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Position.o Position.cpp

${OBJECTDIR}/Security.o: Security.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_CORE -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Security.o Security.cpp

# Subprojects
.build-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCore.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

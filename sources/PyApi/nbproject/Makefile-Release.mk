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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/BaseExport.o \
	${OBJECTDIR}/ContextExport.o \
	${OBJECTDIR}/Detail.o \
	${OBJECTDIR}/LogExport.o \
	${OBJECTDIR}/ModuleExport.o \
	${OBJECTDIR}/ModuleSecurityListExport.o \
	${OBJECTDIR}/OrderParamsExport.o \
	${OBJECTDIR}/PositionExport.o \
	${OBJECTDIR}/PyEngine.o \
	${OBJECTDIR}/PyService.o \
	${OBJECTDIR}/PyStrategy.o \
	${OBJECTDIR}/Script.o \
	${OBJECTDIR}/SecurityExport.o \
	${OBJECTDIR}/ServiceExport.o \
	${OBJECTDIR}/StrategyExport.o \
	${OBJECTDIR}/TradeSystemExport.o


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
LDLIBSOPTIONS=-Wl,-rpath,../Core/dist/Release/GNU-Linux-x86 -L../Core/dist/Release/GNU-Linux-x86 -lCore ../Common/dist/Release/GNU-Linux-x86/libcommon.a -lboost_python -lpython2.6

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}: ../Core/dist/Release/GNU-Linux-x86/libCore.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}: ../Common/dist/Release/GNU-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/BaseExport.o: BaseExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/BaseExport.o BaseExport.cpp

${OBJECTDIR}/ContextExport.o: ContextExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ContextExport.o ContextExport.cpp

${OBJECTDIR}/Detail.o: Detail.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Detail.o Detail.cpp

${OBJECTDIR}/LogExport.o: LogExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/LogExport.o LogExport.cpp

${OBJECTDIR}/ModuleExport.o: ModuleExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ModuleExport.o ModuleExport.cpp

${OBJECTDIR}/ModuleSecurityListExport.o: ModuleSecurityListExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ModuleSecurityListExport.o ModuleSecurityListExport.cpp

${OBJECTDIR}/OrderParamsExport.o: OrderParamsExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/OrderParamsExport.o OrderParamsExport.cpp

${OBJECTDIR}/PositionExport.o: PositionExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PositionExport.o PositionExport.cpp

${OBJECTDIR}/PyEngine.o: PyEngine.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PyEngine.o PyEngine.cpp

${OBJECTDIR}/PyService.o: PyService.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PyService.o PyService.cpp

${OBJECTDIR}/PyStrategy.o: PyStrategy.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PyStrategy.o PyStrategy.cpp

${OBJECTDIR}/Script.o: Script.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Script.o Script.cpp

${OBJECTDIR}/SecurityExport.o: SecurityExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SecurityExport.o SecurityExport.cpp

${OBJECTDIR}/ServiceExport.o: ServiceExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ServiceExport.o ServiceExport.cpp

${OBJECTDIR}/StrategyExport.o: StrategyExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/StrategyExport.o StrategyExport.cpp

${OBJECTDIR}/TradeSystemExport.o: TradeSystemExport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TradeSystemExport.o TradeSystemExport.cpp

# Subprojects
.build-subprojects:
	cd ../Core && ${MAKE}  -f Makefile CONF=Release
	cd ../Common && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../Core && ${MAKE}  -f Makefile CONF=Release clean
	cd ../Common && ${MAKE}  -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

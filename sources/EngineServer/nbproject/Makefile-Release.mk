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
	${OBJECTDIR}/_ext/2108356922/Assert.o \
	${OBJECTDIR}/Main.o \
	${OBJECTDIR}/Server.o \
	${OBJECTDIR}/Service.o \
	${OBJECTDIR}/Client.o \
	${OBJECTDIR}/Settings.o


# C Compiler Flags
CFLAGS=-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=../Common/dist/Release/GNU-Linux-x86/libcommon.a ../EngineService/dist/Release/GNU-Linux-x86/libEngineService.a -Wl,-rpath,../Core/dist/Release/GNU-Linux-x86 -L../Core/dist/Release/GNU-Linux-x86 -lCore -Wl,-rpath,../Engine/dist/Release/GNU-Linux-x86 -L../Engine/dist/Release/GNU-Linux-x86 -lEngine -lboost_system -lboost_filesystem -lboost_regex -lboost_thread -lprotobuf

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine: ../Common/dist/Release/GNU-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine: ../Core/dist/Release/GNU-Linux-x86/libCore.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine: ../Engine/dist/Release/GNU-Linux-x86/libEngine.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine: ../EngineService/dist/Release/GNU-Linux-x86/libEngineService.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine ${OBJECTFILES} ${LDLIBSOPTIONS} -ltcmalloc

${OBJECTDIR}/_ext/2108356922/Assert.o: ../Common/Assert.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/2108356922
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/2108356922/Assert.o ../Common/Assert.cpp

${OBJECTDIR}/Main.o: Main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Main.o Main.cpp

${OBJECTDIR}/Server.o: Server.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Server.o Server.cpp

${OBJECTDIR}/Service.o: Service.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Service.o Service.cpp

${OBJECTDIR}/Client.o: Client.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Client.o Client.cpp

${OBJECTDIR}/Settings.o: Settings.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Settings.o Settings.cpp

# Subprojects
.build-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Release
	cd ../Core && ${MAKE}  -f Makefile CONF=Release
	cd ../Engine && ${MAKE}  -f Makefile CONF=Release
	cd ../Common && ${MAKE}  -f Makefile CONF=Release
	cd ../Core && ${MAKE}  -f Makefile CONF=Release
	cd ../EngineService && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/RobotEngine

# Subprojects
.clean-subprojects:
	cd ../Common && ${MAKE}  -f Makefile CONF=Release clean
	cd ../Core && ${MAKE}  -f Makefile CONF=Release clean
	cd ../Engine && ${MAKE}  -f Makefile CONF=Release clean
	cd ../Common && ${MAKE}  -f Makefile CONF=Release clean
	cd ../Core && ${MAKE}  -f Makefile CONF=Release clean
	cd ../EngineService && ${MAKE}  -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

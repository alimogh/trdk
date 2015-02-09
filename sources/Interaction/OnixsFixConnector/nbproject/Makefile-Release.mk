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
	${OBJECTDIR}/AlpariStream.o \
	${OBJECTDIR}/CurrenexStream.o \
	${OBJECTDIR}/CurrenexTrading.o \
	${OBJECTDIR}/FixSession.o \
	${OBJECTDIR}/FixStream.o \
	${OBJECTDIR}/FixTrading.o \
	${OBJECTDIR}/FxAllStream.o \
	${OBJECTDIR}/FxAllTrading.o \
	${OBJECTDIR}/HotspotTrading.o


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
LDLIBSOPTIONS=../../Common/dist/Release/GNU-Linux-x86/libcommon.a -Wl,-rpath,../../Core/dist/Release/GNU-Linux-x86 -L../../Core/dist/Release/GNU-Linux-x86 -lCore ../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/lib/libffCppFixEngine.so

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libOnixsFixConnector.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libOnixsFixConnector.${CND_DLIB_EXT}: ../../Common/dist/Release/GNU-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libOnixsFixConnector.${CND_DLIB_EXT}: ../../Core/dist/Release/GNU-Linux-x86/libCore.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libOnixsFixConnector.${CND_DLIB_EXT}: ../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/lib/libffCppFixEngine.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libOnixsFixConnector.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libOnixsFixConnector.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/AlpariStream.o: AlpariStream.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/AlpariStream.o AlpariStream.cpp

${OBJECTDIR}/CurrenexStream.o: CurrenexStream.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CurrenexStream.o CurrenexStream.cpp

${OBJECTDIR}/CurrenexTrading.o: CurrenexTrading.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CurrenexTrading.o CurrenexTrading.cpp

${OBJECTDIR}/FixSession.o: FixSession.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FixSession.o FixSession.cpp

${OBJECTDIR}/FixStream.o: FixStream.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FixStream.o FixStream.cpp

${OBJECTDIR}/FixTrading.o: FixTrading.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FixTrading.o FixTrading.cpp

${OBJECTDIR}/FxAllStream.o: FxAllStream.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FxAllStream.o FxAllStream.cpp

${OBJECTDIR}/FxAllTrading.o: FxAllTrading.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FxAllTrading.o FxAllTrading.cpp

${OBJECTDIR}/HotspotTrading.o: HotspotTrading.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I../.. -I../../../externals/OnixS.FixEngineCpp-RHEL70-gcc482-x64-3_17_0_0/include -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/HotspotTrading.o HotspotTrading.cpp

# Subprojects
.build-subprojects:
	cd ../../Common && ${MAKE}  -f Makefile CONF=Release
	cd ../../Core && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libOnixsFixConnector.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../../Common && ${MAKE}  -f Makefile CONF=Release clean
	cd ../../Core && ${MAKE}  -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

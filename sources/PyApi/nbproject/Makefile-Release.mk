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
	${OBJECTDIR}/ScriptEngine.o \
	${OBJECTDIR}/PyApi.o \
	${OBJECTDIR}/Api.o \
	${OBJECTDIR}/PositionWrapper.o


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
LDLIBSOPTIONS=-L/usr/local/boost/boost_1_51/lib -Wl,-rpath,../Core/dist/Release/GNU_4.7.1-Linux-x86 -L../Core/dist/Release/GNU_4.7.1-Linux-x86 -lCore ../Common/dist/Release/GNU_4.7.1-Linux-x86/libcommon.a -lboost_python

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}: ../Core/dist/Release/GNU_4.7.1-Linux-x86/libCore.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}: ../Common/dist/Release/GNU_4.7.1-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -shared -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libPyApi.${CND_DLIB_EXT} -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/ScriptEngine.o: ScriptEngine.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/local/boost/boost_1_51/include -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/ScriptEngine.o ScriptEngine.cpp

${OBJECTDIR}/PyApi.o: PyApi.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/local/boost/boost_1_51/include -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/PyApi.o PyApi.cpp

${OBJECTDIR}/Api.o: Api.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/local/boost/boost_1_51/include -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Api.o Api.cpp

${OBJECTDIR}/PositionWrapper.o: PositionWrapper.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_PYAPI -I.. -I/usr/local/boost/boost_1_51/include -I/usr/include/python2.6 -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/PositionWrapper.o PositionWrapper.cpp

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

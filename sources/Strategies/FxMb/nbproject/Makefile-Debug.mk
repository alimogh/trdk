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
	${OBJECTDIR}/TriangulationWithDirection.o \
	${OBJECTDIR}/TriangulationWithDirectionStatService.o \
	${OBJECTDIR}/TriangulationWithDirectionReport.o \
	${OBJECTDIR}/TriangulationWithDirectionTriangle.o


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
LDLIBSOPTIONS=../../Common/dist/Debug/GNU-Linux-x86/libcommon_dbg.a -Wl,-rpath,../../Core/dist/Debug/GNU-Linux-x86 -L../../Core/dist/Debug/GNU-Linux-x86 -lCore_dbg

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFxMb_dbg.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFxMb_dbg.${CND_DLIB_EXT}: ../../Common/dist/Debug/GNU-Linux-x86/libcommon_dbg.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFxMb_dbg.${CND_DLIB_EXT}: ../../Core/dist/Debug/GNU-Linux-x86/libCore_dbg.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFxMb_dbg.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFxMb_dbg.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/FxArb1.o: FxArb1.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -D_DEBUG -I../.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FxArb1.o FxArb1.cpp


${OBJECTDIR}/TriangulationWithDirection.o: TriangulationWithDirection.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -D_DEBUG -I../.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TriangulationWithDirection.o TriangulationWithDirection.cpp

${OBJECTDIR}/TriangulationWithDirectionStatService.o: TriangulationWithDirectionStatService.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -D_DEBUG -I../.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TriangulationWithDirectionStatService.o TriangulationWithDirectionStatService.cpp

${OBJECTDIR}/TriangulationWithDirectionReport.o: TriangulationWithDirectionReport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -D_DEBUG -I../.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TriangulationWithDirectionReport.o TriangulationWithDirectionReport.cpp

${OBJECTDIR}/TriangulationWithDirectionTriangle.o: TriangulationWithDirectionTriangle.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -D_DEBUG -I../.. -std=c++11 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TriangulationWithDirectionTriangle.o TriangulationWithDirectionTriangle.cpp

# Subprojects
.build-subprojects:
	cd ../../Common && ${MAKE}  -f Makefile CONF=Debug
	cd ../../Core && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFxMb_dbg.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../../Common && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../../Core && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

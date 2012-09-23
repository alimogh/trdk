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
	${OBJECTDIR}/Methods.o \
	${OBJECTDIR}/Service.o \
	${OBJECTDIR}/Api.o


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
LDLIBSOPTIONS=-L/usr/local/boost/boost_1_51/lib ../../Common/dist/Release/GNU_4.7.1-Linux-x86/libcommon.a -Wl,-rpath,../../Core/dist/Release/GNU_4.7.1-Linux-x86 -L../../Core/dist/Release/GNU_4.7.1-Linux-x86 -lCore

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libService.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libService.${CND_DLIB_EXT}: ../../Common/dist/Release/GNU_4.7.1-Linux-x86/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libService.${CND_DLIB_EXT}: ../../Core/dist/Release/GNU_4.7.1-Linux-x86/libCore.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libService.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -shared -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libService.${CND_DLIB_EXT} -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/Methods.o: Methods.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_GATEWAY_SERVICE -I../.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Methods.o Methods.cpp

${OBJECTDIR}/Service.o: Service.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_GATEWAY_SERVICE -I../.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Service.o Service.cpp

${OBJECTDIR}/Api.o: Api.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -DTRADER_GATEWAY_SERVICE -I../.. -I/usr/local/boost/boost_1_51/include -std=c++11 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/Api.o Api.cpp

# Subprojects
.build-subprojects:
	cd ../../Common && ${MAKE}  -f Makefile CONF=Release
	cd ../../Core && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libService.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../../Common && ${MAKE}  -f Makefile CONF=Release clean
	cd ../../Core && ${MAKE}  -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

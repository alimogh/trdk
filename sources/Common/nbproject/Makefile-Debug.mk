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
	${OBJECTDIR}/Currency.o \
	${OBJECTDIR}/Defaults.o \
	${OBJECTDIR}/Exception.o \
	${OBJECTDIR}/FileSystemChangeNotificator.o \
	${OBJECTDIR}/Ini.o \
	${OBJECTDIR}/Prec.o \
	${OBJECTDIR}/Symbol.o \
	${OBJECTDIR}/SysError.o \
	${OBJECTDIR}/TimeMeasurement.o \
	${OBJECTDIR}/Util.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-fpic
CXXFLAGS=-fpic

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon_dbg.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon_dbg.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon_dbg.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon_dbg.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon_dbg.a

${OBJECTDIR}/Currency.o: Currency.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Currency.o Currency.cpp

${OBJECTDIR}/Defaults.o: Defaults.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Defaults.o Defaults.cpp

${OBJECTDIR}/Exception.o: Exception.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Exception.o Exception.cpp

${OBJECTDIR}/FileSystemChangeNotificator.o: FileSystemChangeNotificator.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FileSystemChangeNotificator.o FileSystemChangeNotificator.cpp

${OBJECTDIR}/Ini.o: Ini.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Ini.o Ini.cpp

${OBJECTDIR}/Prec.o: Prec.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Prec.o Prec.cpp

${OBJECTDIR}/Symbol.o: Symbol.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Symbol.o Symbol.cpp

${OBJECTDIR}/SysError.o: SysError.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SysError.o SysError.cpp

${OBJECTDIR}/TimeMeasurement.o: TimeMeasurement.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TimeMeasurement.o TimeMeasurement.cpp

${OBJECTDIR}/Util.o: Util.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Util.o Util.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon_dbg.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

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
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/1207213382/Error.o \
	${OBJECTDIR}/_ext/1207213382/Exception.o \
	${OBJECTDIR}/_ext/1207213382/Defaults.o \
	${OBJECTDIR}/_ext/1207213382/FileSystemChangeNotificator.o \
	${OBJECTDIR}/_ext/1207213382/IniFile.o \
	${OBJECTDIR}/_ext/1207213382/Util.o


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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon.a

${OBJECTDIR}/_ext/1207213382/Error.o: /mnt/hgfs/Projects/Trader/sources/Common/Error.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1207213382
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1207213382/Error.o /mnt/hgfs/Projects/Trader/sources/Common/Error.cpp

${OBJECTDIR}/_ext/1207213382/Exception.o: /mnt/hgfs/Projects/Trader/sources/Common/Exception.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1207213382
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1207213382/Exception.o /mnt/hgfs/Projects/Trader/sources/Common/Exception.cpp

${OBJECTDIR}/_ext/1207213382/Defaults.o: /mnt/hgfs/Projects/Trader/sources/Common/Defaults.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1207213382
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1207213382/Defaults.o /mnt/hgfs/Projects/Trader/sources/Common/Defaults.cpp

${OBJECTDIR}/_ext/1207213382/FileSystemChangeNotificator.o: /mnt/hgfs/Projects/Trader/sources/Common/FileSystemChangeNotificator.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1207213382
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1207213382/FileSystemChangeNotificator.o /mnt/hgfs/Projects/Trader/sources/Common/FileSystemChangeNotificator.cpp

${OBJECTDIR}/_ext/1207213382/IniFile.o: /mnt/hgfs/Projects/Trader/sources/Common/IniFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1207213382
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1207213382/IniFile.o /mnt/hgfs/Projects/Trader/sources/Common/IniFile.cpp

${OBJECTDIR}/_ext/1207213382/Util.o: /mnt/hgfs/Projects/Trader/sources/Common/Util.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1207213382
	${RM} $@.d
	$(COMPILE.cc) -g -Werror -DBOOST_ENABLE_ASSERT_HANDLER -DDEV_VER -DTRADER_COMMON -D_DEBUG -I.. -I/usr/local/boost/boost_1_51/include -std=c++11 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1207213382/Util.o /mnt/hgfs/Projects/Trader/sources/Common/Util.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcommon.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

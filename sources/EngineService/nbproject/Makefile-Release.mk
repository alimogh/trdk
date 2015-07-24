
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
	${OBJECTDIR}/Utils.o \
	${OBJECTDIR}/trdk.EngineService.pb.o \
	${OBJECTDIR}/trdk.EngineService.MarketData.pb.o \
	${OBJECTDIR}/trdk.EngineService.MarketDataSource.pb.o \
	${OBJECTDIR}/trdk.EngineService.Control.pb.o \
	${OBJECTDIR}/trdk.EngineService.DropCopy.pb.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-fpic -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
CXXFLAGS=-fpic -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngineService.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngineService.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngineService.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngineService.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngineService.a

${OBJECTDIR}/Utils.o: Utils.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Utils.o Utils.cpp

${OBJECTDIR}/trdk.EngineService.pb.o: trdk.EngineService.pb.cc 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trdk.EngineService.pb.o trdk.EngineService.pb.cc

${OBJECTDIR}/trdk.EngineService.MarketData.pb.o: trdk.EngineService.MarketData.pb.cc 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trdk.EngineService.MarketData.pb.o trdk.EngineService.MarketData.pb.cc

${OBJECTDIR}/trdk.EngineService.MarketDataSource.pb.o: trdk.EngineService.MarketDataSource.pb.cc 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trdk.EngineService.MarketDataSource.pb.o trdk.EngineService.MarketDataSource.pb.cc

${OBJECTDIR}/trdk.EngineService.Control.pb.o: trdk.EngineService.Control.pb.cc 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trdk.EngineService.Control.pb.o trdk.EngineService.Control.pb.cc

${OBJECTDIR}/trdk.EngineService.DropCopy.pb.o: trdk.EngineService.DropCopy.pb.cc 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Werror -DBOOST_DISABLE_ASSERTS -DNDEBUG -DNTEST -I.. -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trdk.EngineService.DropCopy.pb.o trdk.EngineService.DropCopy.pb.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libEngineService.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

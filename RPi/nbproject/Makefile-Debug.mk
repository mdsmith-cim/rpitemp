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
CND_PLATFORM=GNU-Linux
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
	${OBJECTDIR}/RHCRC.o \
	${OBJECTDIR}/RHDatagram.o \
	${OBJECTDIR}/RHGenericDriver.o \
	${OBJECTDIR}/RHGenericSPI.o \
	${OBJECTDIR}/RHHardwareSPI.o \
	${OBJECTDIR}/RHNRFSPIDriver.o \
	${OBJECTDIR}/RHReliableDatagram.o \
	${OBJECTDIR}/RHSPIDriver.o \
	${OBJECTDIR}/RH_NRF24.o \
	${OBJECTDIR}/RasPi.o \
	${OBJECTDIR}/main.o


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
LDLIBSOPTIONS=-lwiringPi

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/rpi_sensor_master

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/rpi_sensor_master: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/rpi_sensor_master ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/RHCRC.o: RHCRC.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHCRC.o RHCRC.cpp

${OBJECTDIR}/RHDatagram.o: RHDatagram.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHDatagram.o RHDatagram.cpp

${OBJECTDIR}/RHGenericDriver.o: RHGenericDriver.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHGenericDriver.o RHGenericDriver.cpp

${OBJECTDIR}/RHGenericSPI.o: RHGenericSPI.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHGenericSPI.o RHGenericSPI.cpp

${OBJECTDIR}/RHHardwareSPI.o: RHHardwareSPI.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHHardwareSPI.o RHHardwareSPI.cpp

${OBJECTDIR}/RHNRFSPIDriver.o: RHNRFSPIDriver.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHNRFSPIDriver.o RHNRFSPIDriver.cpp

${OBJECTDIR}/RHReliableDatagram.o: RHReliableDatagram.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHReliableDatagram.o RHReliableDatagram.cpp

${OBJECTDIR}/RHSPIDriver.o: RHSPIDriver.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RHSPIDriver.o RHSPIDriver.cpp

${OBJECTDIR}/RH_NRF24.o: RH_NRF24.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RH_NRF24.o RH_NRF24.cpp

${OBJECTDIR}/RasPi.o: RasPi.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/RasPi.o RasPi.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -O -Wall -DRASPBERRY_PI -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/rpi_sensor_master

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

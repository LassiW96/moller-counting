set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Set build type (RelWithDebInfo is default)")

set(DEV_WARNING "cmake build of MOLLER-Offline is still in development. Please test and report issues")
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(moller)

#----------------------------------------------------------------------------
# Names of source files and target libraries.
# You do want to modify this section.

# The name of your package.
# The generated shared library will be named lib$(PACKAGE).so/dylib
#set(PACKAGE moller)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
ENDIF("${isSystemDir}" STREQUAL "-1")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

#----------------------------------------------------------------------------

## Disable in-source builds!
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "cmake cannot build MOLLER in the root directory: "
    "${PROJECT_SOURCE_DIR}\nsince it conflicts with our old Makefile."
    " Create a build directory anywhere, change to it and re-run cmake."
    )
endif()



# Sources.
# List all your source files here. They will be put into a shared library
# that can be loaded from a script.
# List only the implementation files (*.cxx). For every implementation file
# there must be a corresponding header file (*.h).
set(sources
  MOLLERSpectrometer.cxx
  MOLLERGEMModule.cxx
  MOLLERGEMTrackerBase.cxx
  MOLLERGEMSpectrometerTracker.cxx
  MPDModule.cxx
  MOLLERGenericDetector.cxx MOLLERElement.cxx MOLLERData.cxx MOLLERManager.cxx
  )

# Headers.
# Leave the following line untouched.
string(REPLACE .cxx .h headers "${sources}")
list(APPEND headers "DebugDef.h" "Helper.h")

# If you have any extra headers needed for building the ROOT dictionary,
# add them here. Otherwise leave this line commented out.
#list(APPEND headers UserClassHeader.h)

# If you have other headers NOT needed for the ROOT dictionary but needed for
# the library, add them here as shown in the line commented out below.
#set(allheaders ${headers} StandaloneHeader1.h StandaloneHeader2.h)

#------------------------------------------------------------------------------
#FIXME: should be configure'd:
set(EXTRAWARN ON CACHE BOOL "g++ Wextra warn options if supported")

##----------------------------------------------------------------------------
set(VERBOSE ON CACHE BOOL "Compile extra code for printing verbose messages")
set(TESTCODE OFF CACHE BOOL "Compile extra diagnostic code (extra computations and global variables")
set(MCDATA OFF CACHE BOOL "Compile support code for MC input data")
set(CXXMAXERRORS 0 CACHE STRING "Maximum number of allowed errors before cxx stops")
list(APPEND MOLLEREXTRADEF_LIST VERBOSE TESTCODE MCDATA)

#list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake/Modules")
#list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
#----------------------------------------------------------------------------
# Find Podd, if necessary, and load its CMake module
if(NOT TARGET Podd::HallA)
  if(DEFINED ENV{ANALYZER})
    set(CMAKE_PREFIX_PATH "$ENV{ANALYZER}" "${CMAKE_PREFIX_PATH}")
  endif()

  find_package(Podd 1.7 REQUIRED)
endif()
include(PoddCMakeEnv)
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS RIO)
message(STATUS "ROOT ${ROOT_VERSION} found at ${ROOT_BINDIR}")
#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
#include(${ROOT_USE_FILE})

##----------------------------------------------------------------------------
## Setup the PODD Analyzer
#find_package(Podd 1.7 REQUIRED)

## Check for SStream support
include(TestForSSTREAM)
if(CMAKE_HAS_ANSI_STRING_STREAM)
  set(HAS_SSTREAM ON)
  list(APPEND MOLLEREXTRADEF_LIST HAS_SSTREAM)
endif()

##----------------------------------------------------------------------------
## OS Type (not sure what exactly uses this, but since it's in the Makefile
## I'll just copy it here too...
set(MOLLER_LINKFLAGS "")
if(UNIX) ## Can be anything from Linux to Mac OS X
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
  if(APPLE)
    set(MACVERS ON)
    list(APPEND MOLLEREXTRADEF_LIST MACVERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
    if(EXTRAWARN)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    endif()
    set(MOLLER_LINKFLAGS "${MOLLER_LINKFLAGS} -Wl,-undefined,dynamic_lookup")
  else()
    set(LINUXVERS ON)
    list(APPEND MOLLEREXTRADEF_LIST LINUXVERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
    if(EXTRAWARN AND CMAKE_COMPILER_IS_GNUCXX AND NOT
        CMAKE_CXX_COMPILER_VERSION LESS 4)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    endif()
    if(CXXMAXERRORS GREATER 0)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=${CXXMAXERRORS}")
    endif()
  endif()
endif() ## I guess no Windows support?



##----------------------------------------------------------------------------
## Copy over some flags we have set in the Makefile
if(VERBOSE)
  list(APPEND VERBOSE)
endif()

#set_diagnostic_flags(WALL WEXTRA)
#report_build_info()


include_directories(${ROOT_INCLUDE_DIR} ${Podd_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")

set(PROJECT_DICTIONARY ${PROJECT_NAME}Dict)
add_library(${PROJECT_NAME} SHARED ${sources} ${headers} ${PROJECT_DICTIONARY}.cxx )
target_include_directories(${PROJECT_NAME} PRIVATE 
  $<INSTALL_INTERFACE:$CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
include(GNUInstallDirs)
#ROOT_GENERATE_DICTIONARY( ${PROJECT_DICTIONARY} ${headers} OPTIONS -I${PROJECT_SOURCE_DIR} MODULE ${PROJECT_NAME} LINKDEF ${PROJECT_NAME}_LinkDef.h )
# ROOT dictionary
build_root_dictionary(${PROJECT_NAME} ${headers}
  TARGETS ${PROJECT_NAME}
  LINKDEF ${PROJECT_NAME}_LinkDef.h
  )
set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS " ${MOLLER_LINKFLAGS}")


target_link_libraries(${PROJECT_NAME} ${ROOT_LIBRARIES} Podd::Podd)

foreach(_def ${MOLLEREXTRADEF_LIST})
  if(${${_def}})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${_def})
  endif()
endforeach()

set(REPLAYDIR MOLLER-replay)

if( DEFINED ENV{MOLLER_REPLAY} )
    set( DEFAULT_MOLLER_REPLAY_PATH $ENV{MOLLER_REPLAY} )
else() # look in directory 
   get_filename_component( DEFAULT_MOLLER_REPLAY_PATH ../${REPLAYDIR} ABSOLUTE BASE_DIR ${PROJECT_SOURCE_DIR})
endif()		   

set(MOLLER_REPLAY_PATH ${DEFAULT_MOLLER_REPLAY_PATH} CACHE PATH "Path to ${REPLAYDIR} installation")

# check if moller_replay_path set by default from environment matches MOLLER-replay, adjust if needed:
if( NOT ${MOLLER_REPLAY_PATH} MATCHES "/${REPLAYDIR}$" )
  if( ${MOLLER_REPLAY_PATH} MATCHES "/MOLLER-replay$" )
    message( WARNING "default replay directory ${MOLLER_REPLAY_PATH} from environment matches MOLLER-replay but GEN-replay option was given on the command-line, switching to MOLLER-replay" )
    set(REPLAYDIR MOLLER-replay)
  else() 
    message( WARNING "default replay directory ${MOLLER_REPLAY_PATH} from environment doesn't match a supported configuration, appending ${REPLAYDIR}, YMMV..." )
  endif()
# 
  string(APPEND MOLLER_REPLAY_PATH "/${REPLAYDIR}")
endif()

if(NOT EXISTS ${MOLLER_REPLAY_PATH})
  if( EXISTS ${DEFAULT_MOLLER_REPLAY_PATH} )
    message( WARNING "${REPLAYDIR} not found in ${MOLLER_REPLAY_PATH}, reverting to replay installation found in ${DEFAULT_MOLLER_REPLAY_PATH}" )
    set(MOLLER_REPLAY_PATH ${DEFAULT_MOLLER_REPLAY_PATH} CACHE PATH "Path to ${REPLAYDIR} installation" FORCE)
  elseif(${DEFAULT_MOLLER_REPLAY_PATH} STREQUAL ${MOLLER_REPLAY_PATH})
    message(WARNING "${REPLAYDIR} not found in default location ${DEFAULT_MOLLER_REPLAY_PATH}. You can still build MOLLER-offline, but the \"replay\" and \"scripts\" directories under ${REPLAYDIR} won't be automatically added to ROOT's macro path in the .rootrc file. To fix this, set environment variable MOLLER_REPLAY to point to your ${REPLAYDIR} installation and/or re-run cmake with -DMOLLER_REPLAY_PATH=/path/to/your/custom/${REPLAYDIR}/")
  else()
    message(WARNING "${REPLAYDIR} not found in either its default location ${DEFAULT_MOLLER_REPLAY_PATH} or user-specified location ${MOLLER_REPLAY_PATH}. You can still build MOLLER-offline, but the \"replay\" and \"scripts\" directories under ${REPLAYDIR} won't be automatically added to ROOT's macro path in the .rootrc file. To fix this, set environment variable MOLLER_REPLAY to point to your ${REPLAYDIR} installation and/or re-run cmake with -DMOLLER_REPLAY_PATH=/path/to/your/custom/${REPLAYDIR}/")
  endif()	   
endif()	

if( EXISTS ${MOLLER_REPLAY_PATH} )
    set(MOLLER_MACRO_PATH ".:${MOLLER_REPLAY_PATH}/replay:${MOLLER_REPLAY_PATH}/scripts:${MOLLER_REPLAY_PATH}/onlineGUIconfig: ${MOLLER_REPLAY_PATH}/onlineGUIconfig/scripts")
else()
    set(MOLLER_MACRO_PATH ".")
endif()

configure_file( ${PROJECT_SOURCE_DIR}/mollerenv.csh ${CMAKE_CURRENT_BINARY_DIR}/mollerenv.csh @ONLY)
configure_file( ${PROJECT_SOURCE_DIR}/mollerenv.sh ${CMAKE_CURRENT_BINARY_DIR}/mollerenv.sh)

configure_file( ${PROJECT_SOURCE_DIR}/MOLLER.rootrc ${CMAKE_CURRENT_BINARY_DIR}/.rootrc )
configure_file( ${PROJECT_SOURCE_DIR}/rootlogon_install.C ${PROJECT_BINARY_DIR}/rootlogon.C )

## Time to disable this warning. The cmake build works...
#message(AUTHOR_WARNING ${DEV_WARNING})

#install the library under ${CMAKE_INSTALL_PREFIX}/lib
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#install the relevant directories: DB, replay, scripts in the top-level installation folder (these have been moved to ${REPLAYDIR} repository)
#INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/DB DESTINATION ${CMAKE_INSTALL_PREFIX})
#INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/DB_UVA_EEL_COSMIC_JAN2021 DESTINATION ${CMAKE_INSTALL_PREFIX})
#INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/replay DESTINATION ${CMAKE_INSTALL_PREFIX})
#INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mollerenv.sh DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mollerenv.csh DESTINATION ${CMAKE_INSTALL_BINDIR})
#install all headers under cmake_install_prefix/include
INSTALL(FILES ${headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# install the ROOT dictionary files: eventually we should learn how to do this in a smarter way. Put the .cxx file under "include"
# and put the _rdict.pcm and .rootmap files under "lib". This seems to work. Don't ask me why
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_DICTIONARY}.cxx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/rootlogon.C DESTINATION ${CMAKE_INSTALL_SYSCONFDIR} )

#this creates run_replay_here folder under install prefix
INSTALL(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/run_replay_here)

# put the .rootrc file under run_replay_here
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/.rootrc DESTINATION ${CMAKE_INSTALL_PREFIX}/run_replay_here)

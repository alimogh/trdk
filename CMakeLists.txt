
cmake_minimum_required(VERSION 2.8.11.2)

# http://public.kitware.com/Bug/view.php?id=5811
# http://www.cmake.org/pipermail/cmake/2012-January/048856.html
set(CMAKE_CONFIGURATION_TYPES Release Debug Test)
set(
	CMAKE_CONFIGURATION_TYPES
	"${CMAKE_CONFIGURATION_TYPES}"
	CACHE STRING "Reset the configurations to what we need." FORCE)

# set(CMAKE_VERBOSE_MAKEFILE "ON")

project(TRDK)

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
	set(TRDK_PROJECT_ARCH "x64")
elseif(CMAKE_SIZEOF_VOID_P MATCHES 4)
	set(TRDK_PROJECT_ARCH "x86")
else()
	message(ERROR "Uknown Project Arch with size of pointer ${CMAKE_SIZEOF_VOID_P}.")
	return()
endif()

set(TRDK_SOLUTION_DIR "${CMAKE_SOURCE_DIR}/")
set(TRDK_SOURCES_DIR "${TRDK_SOLUTION_DIR}sources/")
set(TRDK_OUTPUT_DIR "${TRDK_SOLUTION_DIR}output/")
set(TRDK_EXTERNALS_DIR "${TRDK_SOLUTION_DIR}externals/")
set(
	TRDK_BINARY_DIR
	"${TRDK_OUTPUT_DIR}${CMAKE_CXX_COMPILER_ID}/${TRDK_PROJECT_ARCH}/"
	CACHE STRING "Output dir." FORCE)
set(CMAKE_BINARY_DIR "${TRDK_BINARY_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_TEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_TEST "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

################################################################################
## Configurations:

set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -DNTEST -DBOOST_DISABLE_ASSERTS")
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -DDEV_VER -DBOOST_ENABLE_ASSERT_HANDLER")
set(CMAKE_CXX_FLAGS_TEST "-D_TEST -DNDEBUG -DDEV_VER -DBOOST_ENABLE_ASSERT_HANDLER")

################################################################################
## Common definitions:

add_definitions(-DNOMINMAX)
add_definitions(-DTRDK_DISTRIBUTION_STANDALONE)

################################################################################
## Compiler options:

if(MSVC)

	################################################################################
	## Visual studio:
	
	# ????
	# Turn on the ability to create folders to organize projects (.vcproj)
	# It creates "CMakePredefinedTargets" folder by default and adds CMake
	# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj.
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	# Debug Information Format (/Zi - Program Data Base).
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
	# Disable Specific Warnings.
	set(
		CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} /wd\"4503\" /wd\"4275\" /wd\"4251\" /wd\"4250\"")
	# Enable C++ Exceptions: Yes with SEH Exceptions (/EHa).
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
	# Enable Function Level Linking: Yes (/Gy).
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gy")
	# Enable Intrinsic Functions.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Oi")
	# Enable String Pooling.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GF")
	# Precompiler header
	# Defines.
	set(
		CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0600 /DWINVER=0x0600 /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS")
	# Warning Level.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	# Treat Warnings As Errors.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

	# Runtime Library: Multi-threaded (/MT).
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	# Security Check: Disable Security Check (/GS-).
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GS-")

	# Basic Runtime Checks: Both (/RTC1).
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /RTC1")
	# Enable Minimal Rebuild: Yes (/Gm).
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm")
	# Optimization: Disabled (/Od).
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")

	# Enable Minimal Rebuild: Yes (/Gm).
	set(CMAKE_CXX_FLAGS_TEST "${CMAKE_CXX_FLAGS_TEST} /Gm")

elseif(CMAKE_COMPILER_IS_GNUCXX)

    ################################################################################
    ## Common compiler flags:

    set(TRDK_COMPILE_FLAGS "-std=c++11")

    if(NOT CYGWIN)
        # TCMalloc (also see linker)
        set(
            TRDK_COMPILE_FLAGS
            "${TRDK_COMPILE_FLAGS} -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    endif()

    # Generate position-independent code (PIC) suitable for use in a shared library.
    # set(TRDK_COMPILE_FLAGS "${TRDK_COMPILE_FLAGS} -fpic")
    set(CMAKE_C_COMPILE_OPTIONS_PIC "-fpic")
    set(CMAKE_CXX_COMPILE_OPTIONS_PIC "-fpic")
    set(TRDK_COMPILE_FLAGS "${TRDK_COMPILE_FLAGS} ${CMAKE_CXX_COMPILE_OPTIONS_PIC}")

    # Make all warnings into errors.
    set(TRDK_COMPILE_FLAGS "${TRDK_COMPILE_FLAGS} -Werror")

    # Depency tree.
    set(TRDK_COMPILE_FLAGS "${TRDK_COMPILE_FLAGS} -MMD -MP")

    # Produce debugging information for use by GDB.
    set(TRDK_COMPILE_FLAGS "${TRDK_COMPILE_FLAGS} -ggdb3")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TRDK_COMPILE_FLAGS}")

    ################################################################################
    ## Configuration specific compiler flags:

    # Optimize. -O3 turns on all optimizations specified by -O2 and also turns on MORE.
    set(CMAKE_CXX_FLAGS_TEST "${CMAKE_CXX_FLAGS_TEST} -O3")

    # Optimize. -O3 turns on all optimizations specified by -O2 and also turns on MORE.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

    ################################################################################
    ## Common linker flags:

    set(TRDK_LINK_FLAGS)

    if(NOT CYGWIN)
        # TCMalloc (also see compiler)
        set(TRDK_LINK_FLAGS "${TRDK_LINK_FLAGS} -ltcmalloc")
    endif()

    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${TRDK_LINK_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TRDK_LINK_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TRDK_LINK_FLAGS}")

    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "./:/usr/local/lib/")

else()

    ################################################################################
    ## Unknown:

    message(FATAL_ERROR "This project can be built with this compiler!")

endif()

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_TEST "${CMAKE_CXX_FLAGS_TEST}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

################################################################################
## Linker options:

set(CMAKE_DEBUG_POSTFIX _dbg)
set(CMAKE_TEST_POSTFIX _test)

################################################################################
## Boost:

if(WIN32)
	if(NOT EXISTS "$ENV{BOOST_ROOT}")
		message(
			WARNING
			"Environment variable \"BOOST_ROOT\" does not exist!")
	endif()
	set(BOOST_LIBRARYDIR "$ENV{BOOST_ROOT}/lib64-msvc-12.0/")
	set(Boost_USE_STATIC_LIBS ON)
elseif(CYGWIN)
	set(Boost_USE_STATIC_LIBS OFF)
else()
	set(Boost_USE_STATIC_LIBS OFF)
endif()

set(Boost_USE_MULTITHREADED ON)
# set(Boost_USE_STATIC_RUNTIME OFF)
find_package(
	Boost
	1.56.0
	REQUIRED
	COMPONENTS system date_time thread filesystem chrono regex atomic)

include_directories(${Boost_INCLUDE_DIRS})

################################################################################
## Utils:

macro(trdk_add_precompiled_header header precompiled_header source_list)

	foreach(f ${source_list})
        if(f MATCHES "${header}" OR f MATCHES "${precompiled_header}")
            message(FATAL_ERROR "Source List already is has Precompiled Header!")
        endif()
    endforeach()

	if(MSVC)
		get_filename_component(precompiled_basename ${header} NAME_WE)
		set(precompiled_binary "${CMAKE_CURRENT_BINARY_DIR}/${precompiled_basename}.pch")
		set(source_list_for_props ${${source_list}})
		set_source_files_properties(
			${precompiled_header}
                        PROPERTIES COMPILE_FLAGS "/Yc\"${header}\" /Fp\"${precompiled_binary}\""
                        OBJECT_OUTPUTS "${precompiled_binary}")
		set_source_files_properties(
			${source_list_for_props}
			PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${header}\" /Fp\"${precompiled_binary}\""
			OBJECT_DEPENDS "${precompiled_binary}")
		# Add precompiled header to source_list
		list(APPEND ${source_list} ${precompiled_header})
	endif(MSVC)

endmacro(trdk_add_precompiled_header)

################################################################################
## Projects:

add_subdirectory(${TRDK_SOURCES_DIR}/Common)
add_subdirectory(${TRDK_SOURCES_DIR}/Core)

add_subdirectory(${TRDK_SOURCES_DIR}/Engine)
add_subdirectory(${TRDK_SOURCES_DIR}/EngineServer)
add_subdirectory(${TRDK_SOURCES_DIR}/EngineService)

add_subdirectory(${TRDK_SOURCES_DIR}/Interaction/Fake)
add_subdirectory(${TRDK_SOURCES_DIR}/Interaction/OnixsFixConnector)
add_subdirectory(${TRDK_SOURCES_DIR}/Interaction/Itch)

add_subdirectory(${TRDK_SOURCES_DIR}/Strategies/FxMb)

################################################################################

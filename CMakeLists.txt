cmake_minimum_required(VERSION 3.1)
set(EntityX_PYBIND11_MAJOR_VERSION 1)
set(EntityX_PYBIND11_MINOR_VERSION 1)
set(EntityX_PYBIND11_PATCH_VERSION 0)
set(EntityX_PYBIND11_VERSION
    ${EntityX_PYBIND11_MAJOR_VERSION}.${EntityX_PYBIND11_MINOR_VERSION}.${EntityX_PYBIND11_PATCH_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(EntityX_PYBIND11)

message("EntityX_PYBIND11 version ${ENTITYX_PYTHON_VERSION}")

if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ENTITYX_PYTHON_BUILD_TESTING true CACHE BOOL "Enable building of tests.")
set(ENTITYX_PYTHON_BUILD_SHARED false CACHE BOOL "Build shared libraries?")

# Library installation directory
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
set(libdir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

# Default compiler args
if (CMAKE_CXX_COMPILER_ID MATCHES "(GNU|.*Clang)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=sign-compare -std=c++11 -Wno-error=deprecated-declarations -Wno-unused-local-typedefs")
    if (CMAKE_CXX_COMPILER_ID MATCHES "(.*Clang)")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Qunused-arguments")
    endif()
    if (CMAKE_CXX_COMPILER_ID MATCHES "(GNU)")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-local-typedefs")
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-g -Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-g -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL 'MSVC')
    # /Zi - Produces a program database (PDB) that contains type information and symbolic debugging information for use with the debugger.
    # /FS - Allows multiple cl.exe processes to write to the same .pdb file
    # /DEBUG - Enable debug during linking
    # /Od - Disables optimization
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /FS /DEBUG /Od /MDd")
    # /Ox - Full optimization
    set(CMAKE_CXX_FLAGS_RELEASE "/Ox -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Ox /Zi /FS /DEBUG")
endif()

include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
# C++11 feature checks
include(CheckCXXSourceCompiles)

# Set -stdlib=lib++ for the next test. MacOS has libc++ installed, but Linux
# don't necessarily have that.
set(OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if (CMAKE_CXX_COMPILER_ID MATCHES "(GNU|.*Clang)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
check_cxx_source_compiles(
"
#include <memory>

int main() {
    std::shared_ptr<int>();
}
"
ENTITYX_HAVE_CXX11_STDLIB
)

# Revert addition of -stdlib=libc++ if we couldn't build main with memory.
if (NOT ENTITYX_HAVE_CXX11_STDLIB)
    message("-- Not using -stdlib=libc++ (test failed to build)")
    set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
else ()
    message("-- Using -stdlib=libc++")
endif ()

# Misc features
check_include_file("stdint.h" HAVE_STDINT_H)

macro(create_test TARGET_NAME SOURCE)
    add_executable(${TARGET_NAME} ${SOURCE})
    target_link_libraries(
        ${TARGET_NAME}
        entityx_python
        ${ARGN}
        )
    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER "entityx/python/tests/")
    add_test(${TARGET_NAME} ${TARGET_NAME})
endmacro()

if (NOT CMAKE_BUILD_TYPE)
    message("-- Defaulting to release build (use -DCMAKE_BUILD_TYPE:STRING=Debug for debug build)")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory(pybind11)
message(status "** Pybind11 Include: ${PYBIND11_INCLUDE_DIR}")

# Add Python
find_package(PythonLibs 2.7 REQUIRED)
message(status "** Python Include: ${PYTHON_INCLUDE_DIRS}")
message(status "** Python Libraries: ${PYTHON_LIBRARIES}")
if(NOT PYTHONLIBS_FOUND)
    set(PYTHON_ROOT "" CACHE PATH "PYTHON top-level directory")
    message("---> Python 2.7 directory not found. Set PYTHON_ROOT to Pythons's top-level path (containing \"include\" and \"lib\" directories).\n")
endif()

# Add entityx
# TODO(SMA) : Update FindEntityX.cmake to find the version tag, we've tested this
# with version 1.2.0
# find_package(entityx 1.2.0 REQUIRED)
find_package(EntityX REQUIRED)
message(status "** EntityX Include: ${ENTITYX_INCLUDE_DIRS}")
message(status "** EntityX Libraries: ${ENTITYX_LIBRARIES}")
if(NOT entityx_FOUND)
    set(entityx_ROOT "" CACHE PATH "entityx top-level directory")
    message("---> entityx 1.1.2 directory not found. Set Entityx_ROOT to EntityX's top-level path (containing \"include\" and \"lib\" directories).\n")
endif()

set(ENTITYX_INSTALLED_PYTHON_PACKAGE_DIR ${PYTHON_ROOT}/Lib CACHE STRING "Python package directory")

# Define HAVE_ROUND for pymath.h
add_definitions(/DHAVE_ROUND)

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${PYBIND11_INCLUDE_DIR})
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${ENTITYX_INCLUDE_DIRS})

# Generate python configs
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/python/config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/python/config.h
)

# Things to install
set(install_libs entityx_python)
# Inclue headers here so they appear in visual studio.
set(sources entityx/python/PythonSystem.cc
            entityx/python/PythonSystem.h
            entityx/python/PythonScript.hpp
            entityx/python/config.h)
add_library(entityx_python STATIC ${sources})
set_target_properties(entityx_python PROPERTIES DEBUG_POSTFIX -d FOLDER entityx)
target_link_libraries(entityx_python ${ENTITYX_LIBRARIES} ${PYTHON_LIBRARIES})

# Enable python shared builds (untested)
if (ENTITYX_PYTHON_BUILD_SHARED)
    message("-- Building shared libraries (-DENTITYX_PYTHON_BUILD_SHARED=0 to only build static librarires)")
    add_library(entityx_python_shared SHARED ${sources})
    target_link_libraries(entityx_python_shared)
    set_target_properties(entityx_python_shared PROPERTIES
        OUTPUT_NAME entityx_python
        DEBUG_POSTFIX -d
        VERSION ${ENTITYX_VERSION}
        SOVERSION ${ENTITYX_MAJOR_VERSION}
        FOLDER entityx/python/)
    list(APPEND install_libs entityx_python_shared)
endif (ENTITYX_PYTHON_BUILD_SHARED)

if (ENTITYX_PYTHON_BUILD_TESTING)
    enable_testing()
    add_definitions(-DENTITYX_PYTHON_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/entityx/python/")
    create_test(PythonSystem_test entityx/python/PythonSystem_test.cc)
endif (ENTITYX_PYTHON_BUILD_TESTING)

install(
    DIRECTORY "entityx/python/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h*"
    )

install(
    TARGETS ${install_libs}
    LIBRARY DESTINATION "${libdir}"
    ARCHIVE DESTINATION "${libdir}"
    )

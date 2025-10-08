cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

######################
## Project settings ##
######################

project(${SKBUILD_PROJECT_NAME}
    VERSION ${SKBUILD_PROJECT_VERSION}
    LANGUAGES CXX)

# Enforce the C++ standard for the project.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# io_uring is a Linux-only API, so no point in building for other platforms.
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message(FATAL_ERROR "boros can only be built for Linux targets.")
endif()

# Find the required Python components to build the module.
find_package(Python COMPONENTS
    Interpreter
    Development.Module
    ${SKBUILD_SABI_COMPONENT}
    REQUIRED)

#############
## Targets ##
#############

Python_add_library(_impl MODULE
    src/impl/cqueue.hpp
    src/impl/squeue.hpp
    src/impl/utils.hpp

    src/impl/cqueue.cpp
    src/impl/module.cpp
    src/impl/squeue.cpp
    src/impl/utils.cpp

    WITH_SOABI
    USE_SABI ${SKBUILD_SABI_VERSION}
    )

target_compile_options(_impl PRIVATE
    -Wall
    -Werror=array-bounds
    -Werror=implicit-fallthrough
    -Werror=missing-field-initializers
    -Werror=reorder
    -Werror=sign-compare
    -Werror=switch
    -Werror=uninitialized
    -Werror=unused-function
    -Werror=unused-result
    -Werror=unused-variable
    -Wextra
    -Wno-invalid-offsetof
    -Wno-unused-parameter

    -fno-rtti
    -fno-exceptions
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    )
target_compile_definitions(_impl PRIVATE -DPY_SSIZE_T_CLEAN)
target_compile_features(_impl PRIVATE cxx_std_23)
set_property(TARGET _impl PROPERTY
    INTERFACE_POSITION_INDEPENDENT_CODE ON
    INTERPROCEDURAL_OPTIMIZATION ON
    )

install(TARGETS _impl DESTINATION ${SKBUILD_PROJECT_NAME})

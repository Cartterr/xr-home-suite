cmake_minimum_required(VERSION 3.24)

project(xr_home_suite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED XRHS_HOME)
    if(DEFINED ENV{XRHS_HOME})
        file(TO_CMAKE_PATH "$ENV{XRHS_HOME}" XRHS_HOME)
    else()
        set(XRHS_HOME "C:/XRHomeSuite")
    endif()
endif()

set(XRHS_DEPS_DIR "${XRHS_HOME}/deps")
set(XRHS_OPENXR_DIR "${XRHS_DEPS_DIR}/openxr")
set(OPENXR_INCLUDE_DIR "${XRHS_OPENXR_DIR}/include")
set(OPENXR_LOADER_LIB "${XRHS_OPENXR_DIR}/loader/native/x64/release/lib/openxr_loader.lib")
set(OPENXR_LOADER_DLL "${XRHS_OPENXR_DIR}/loader/native/x64/release/bin/openxr_loader.dll")

if(NOT EXISTS "${OPENXR_INCLUDE_DIR}/openxr/openxr.h")
    message(FATAL_ERROR "OpenXR headers are missing at ${OPENXR_INCLUDE_DIR}. Run: python -m xrhs deps sync")
endif()

if(NOT EXISTS "${OPENXR_LOADER_LIB}")
    message(FATAL_ERROR "OpenXR loader library is missing at ${OPENXR_LOADER_LIB}. Run: python -m xrhs deps sync")
endif()

add_library(openxr_loader STATIC IMPORTED GLOBAL)
set_target_properties(openxr_loader PROPERTIES
    IMPORTED_LOCATION "${OPENXR_LOADER_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENXR_INCLUDE_DIR}"
)

add_subdirectory(libs/xrh_core)
add_subdirectory(libs/xrh_openxr)
add_subdirectory(libs/xrh_d3d11)
add_subdirectory(libs/xrh_meta)
add_subdirectory(apps/openxr_probe)

enable_testing()
add_subdirectory(tests)

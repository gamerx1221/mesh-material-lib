cmake_minimum_required(VERSION 3.26)

project(mesh_material_lib VERSION 0.1.0 LANGUAGES CXX)

option(MESH_MATERIAL_ENABLE_MATERIALX "Build MaterialX runtime support" OFF)
option(MESH_MATERIAL_ENABLE_OPENEXR "Build OpenEXR texture adapter" OFF)
option(MESH_MATERIAL_ENABLE_KTX "Build KTX texture adapter" OFF)

add_library(mesh_material_core INTERFACE)
add_library(mesh_material::core ALIAS mesh_material_core)

target_include_directories(mesh_material_core
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(mesh_material_core INTERFACE cxx_std_20)

if(MESH_MATERIAL_ENABLE_MATERIALX)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/MaterialX/CMakeLists.txt")
        message(FATAL_ERROR "MaterialX submodule is missing. Run: git submodule update --init --recursive")
    endif()
    set(MATERIALX_BUILD_GEN_GLSL OFF CACHE BOOL "" FORCE)
    set(MATERIALX_BUILD_GEN_OSL OFF CACHE BOOL "" FORCE)
    set(MATERIALX_BUILD_GEN_MDL OFF CACHE BOOL "" FORCE)
    set(MATERIALX_BUILD_GEN_MSL OFF CACHE BOOL "" FORCE)
    set(MATERIALX_BUILD_RENDER OFF CACHE BOOL "" FORCE)
    set(MATERIALX_BUILD_RENDER_PLATFORMS OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/MaterialX EXCLUDE_FROM_ALL)
endif()

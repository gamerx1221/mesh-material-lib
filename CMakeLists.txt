cmake_minimum_required(VERSION 3.26)

project(mesh_material_lib VERSION 0.1.0 LANGUAGES CXX)

option(MESH_MATERIAL_ENABLE_MATERIALX "Build MaterialX runtime support" OFF)
option(MESH_MATERIAL_BUILD_TESTS "Build material library tests" ON)
option(MESH_MATERIAL_ENABLE_OPENEXR "Build OpenEXR texture adapter" OFF)
option(MESH_MATERIAL_ENABLE_KTX "Build KTX texture adapter" OFF)

add_library(mesh_material_core STATIC
    src/Material.cpp
    src/BuiltinTranslationLayers.cpp)
add_library(mesh_material::core ALIAS mesh_material_core)

target_include_directories(mesh_material_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(mesh_material_core PUBLIC cxx_std_20)

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
    set(MATERIALX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/MaterialX EXCLUDE_FROM_ALL)

    add_library(mesh_material_materialx STATIC
        src/MaterialXAdapter.cpp
        src/MaterialXRuntime.cpp)
    add_library(mesh_material::materialx ALIAS mesh_material_materialx)
    target_include_directories(mesh_material_materialx PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(mesh_material_materialx PUBLIC mesh_material::core MaterialXCore MaterialXFormat)
    target_compile_definitions(mesh_material_materialx PRIVATE
        MESH_MATERIAL_MATERIALX_LIBRARY_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/third_party/MaterialX/libraries")
    target_compile_features(mesh_material_materialx PUBLIC cxx_std_20)
endif()

if(MESH_MATERIAL_BUILD_TESTS AND PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_executable(mesh_material_core_tests tests/MaterialTests.cpp)
    target_link_libraries(mesh_material_core_tests PRIVATE mesh_material::core)
    add_test(NAME mesh_material_core_tests COMMAND mesh_material_core_tests)
    if(TARGET mesh_material_materialx)
        add_executable(mesh_material_materialx_tests tests/MaterialXTests.cpp)
        target_link_libraries(mesh_material_materialx_tests PRIVATE mesh_material::materialx)
        add_test(NAME mesh_material_materialx_tests COMMAND mesh_material_materialx_tests)
    endif()
endif()

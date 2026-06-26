cmake_minimum_required(VERSION 3.22.1)
project(Kronos3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ANDROID)
    set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
    set(CMAKE_SYSTEM_NAME Android)
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/opengl
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
)

# Core math library
file(GLOB_RECURSE KRONOS_MATH_SOURCES core/math/*.cpp)
add_library(kronos_math STATIC ${KRONOS_MATH_SOURCES})
target_compile_options(kronos_math PRIVATE -DKRONOS_MATH_EXPORTS)

# Mesh (BMesh + Subdiv)
file(GLOB_RECURSE KRONOS_MESH_SOURCES core/mesh/*.cpp)
add_library(kronos_mesh STATIC ${KRONOS_MESH_SOURCES})
target_link_libraries(kronos_mesh PUBLIC kronos_math)

# Modifiers
file(GLOB_RECURSE KRONOS_MODIFIERS_SOURCES core/modifiers/*.cpp)
add_library(kronos_modifiers STATIC ${KRONOS_MODIFIERS_SOURCES})
target_link_libraries(kronos_modifiers PUBLIC kronos_mesh kronos_math)

# Cycles CPU
file(GLOB_RECURSE KRONOS_CYCLES_SOURCES core/cycles/*.cpp)
add_library(kronos_cycles STATIC ${KRONOS_CYCLES_SOURCES})
target_link_libraries(kronos_cycles PUBLIC kronos_math kronos_mesh)

# Vulkan Renderer
file(GLOB_RECURSE KRONOS_VULKAN_SOURCES vulkan/*.cpp)
add_library(kronos_vulkan STATIC ${KRONOS_VULKAN_SOURCES})
target_link_libraries(kronos_vulkan PUBLIC kronos_math kronos_mesh)
target_link_libraries(kronos_vulkan PUBLIC vulkan nativewindow)
add_definitions(-DKRONOS_VULKAN)

# OpenGL ES 3.2 Renderer
file(GLOB_RECURSE KRONOS_GLES_SOURCES opengl/*.cpp)
add_library(kronos_gles STATIC ${KRONOS_GLES_SOURCES})
target_link_libraries(kronos_gles PUBLIC kronos_math kronos_mesh)
target_link_libraries(kronos_gles PUBLIC GLESv3 EGL)

# JNI Bridge (shared library)
file(GLOB_RECURSE KRONOS_JNI_SOURCES jni/*.cpp)
add_library(kronos_jni SHARED ${KRONOS_JNI_SOURCES})
target_link_libraries(kronos_jni PUBLIC
    kronos_math
    kronos_mesh
    kronos_modifiers
    kronos_cycles
    kronos_vulkan
    kronos_gles
    android
    log
    GLESv3
    EGL
    nativewindow
)
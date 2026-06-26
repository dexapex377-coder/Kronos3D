cmake_minimum_required(VERSION 3.22.1)
project(Kronos3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ANDROID)
    set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
    set(CMAKE_SYSTEM_NAME Android)
endif()

# Find Vulkan via NDK (not find_package - no .cmake in NDK)
find_library(Vulkan_LIBRARY vulkan)
find_path(Vulkan_INCLUDE_DIR vulkan/vulkan.h)

# Find OpenGL ES and EGL
find_library(GLESv3_LIBRARY GLESv3)
find_library(EGL_LIBRARY EGL)
find_library(NATIVE_WINDOW_LIBRARY nativewindow)

if(NOT GLESv3_LIBRARY)
    message(WARNING "GLESv3 not found, trying GLESv2")
    find_library(GLESv3_LIBRARY GLESv2)
endif()

message(STATUS "Vulkan lib: ${Vulkan_LIBRARY}")
message(STATUS "Vulkan include: ${Vulkan_INCLUDE_DIR}")
message(STATUS "GLESv3: ${GLESv3_LIBRARY}")
message(STATUS "EGL: ${EGL_LIBRARY}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/opengl
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
    ${Vulkan_INCLUDE_DIR}
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

# Vulkan Renderer (optional - only if Vulkan found)
if(Vulkan_LIBRARY AND Vulkan_INCLUDE_DIR)
    message(STATUS "Vulkan found, building Vulkan renderer")
    file(GLOB_RECURSE KRONOS_VULKAN_SOURCES vulkan/*.cpp)
    add_library(kronos_vulkan STATIC ${KRONOS_VULKAN_SOURCES})
    target_link_libraries(kronos_vulkan PUBLIC
        kronos_math kronos_mesh
        ${Vulkan_LIBRARY}
        ${NATIVE_WINDOW_LIBRARY}
    )
    target_include_directories(kronos_vulkan PUBLIC ${Vulkan_INCLUDE_DIR})
    add_definitions(-DKRONOS_VULKAN)
else()
    message(WARNING "Vulkan not found, disabling Vulkan renderer")
    add_library(kronos_vulkan STATIC)
    add_definitions(-DKRONOS_NO_VULKAN)
endif()

# OpenGL ES 3.2 Renderer
file(GLOB_RECURSE KRONOS_GLES_SOURCES opengl/*.cpp)
add_library(kronos_gles STATIC ${KRONOS_GLES_SOURCES})
target_link_libraries(kronos_gles PUBLIC
    kronos_math kronos_mesh
    ${GLESv3_LIBRARY}
    ${EGL_LIBRARY}
)

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
    ${GLESv3_LIBRARY}
    ${EGL_LIBRARY}
    ${NATIVE_WINDOW_LIBRARY}
)

if(ANDROID)
    target_link_options(kronos_jni PRIVATE -Wl,--no-warn-mismatch)
endif()
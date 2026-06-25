cmake_minimum_required(VERSION 3.22.1)
project(Kronos3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Android NDK specifics
if(ANDROID)
    set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
    set(CMAKE_SYSTEM_NAME Android)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find OpenGL ES 3.2
find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/opengl
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
)

# Core math library (blenlib port)
file(GLOB_RECURSE KRONOS_MATH_SOURCES
    core/math/*.cpp
    core/math/*.h
    core/math/*.inl
)

add_library(kronos_math STATIC ${KRONOS_MATH_SOURCES})
target_compile_options(kronos_math PRIVATE -DKRONOS_MATH_EXPORTS)

# BMesh + modifiers
file(GLOB_RECURSE KRONOS_MESH_SOURCES
    core/mesh/*.cpp
    core/mesh/*.h
)

add_library(kronos_mesh STATIC ${KRONOS_MESH_SOURCES})
target_link_libraries(kronos_mesh PUBLIC kronos_math)

# Modifiers stack
file(GLOB_RECURSE KRONOS_MODIFIERS_SOURCES
    core/modifiers/*.cpp
    core/modifiers/*.h
)

add_library(kronos_modifiers STATIC ${KRONOS_MODIFIERS_SOURCES})
target_link_libraries(kronos_modifiers PUBLIC kronos_mesh kronos_math)

# Cycles CPU kernels (ported from Blender 4.5)
file(GLOB_RECURSE KRONOS_CYCLES_SOURCES
    core/cycles/*.cpp
    core/cycles/*.h
)

add_library(kronos_cycles STATIC ${KRONOS_CYCLES_SOURCES})
target_link_libraries(kronos_cycles PUBLIC kronos_math kronos_mesh)
target_compile_options(kronos_cycles PRIVATE -DKERNEL_SSE2 -DKERNEL_NEON)

# Vulkan Renderer
file(GLOB_RECURSE KRONOS_VULKAN_SOURCES
    vulkan/*.cpp
    vulkan/*.h
)

add_library(kronos_vulkan STATIC ${KRONOS_VULKAN_SOURCES})
target_link_libraries(kronos_vulkan PUBLIC kronos_math kronos_mesh Vulkan::Vulkan)
target_include_directories(kronos_vulkan PUBLIC ${Vulkan_INCLUDE_DIRS})

# OpenGL ES 3.2 Renderer (fallback)
file(GLOB_RECURSE KRONOS_GLES_SOURCES
    opengl/*.cpp
    opengl/*.h
)

add_library(kronos_gles STATIC ${KRONOS_GLES_SOURCES})
target_link_libraries(kronos_gles PUBLIC kronos_math kronos_mesh ${OPENGL_ES_LIBRARIES})

# JNI Bridge
file(GLOB_RECURSE KRONOS_JNI_SOURCES
    jni/*.cpp
    jni/*.h
)

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
    EGL
    GLESv3
)

# Install headers for potential future use
install(DIRECTORY core/math/ DESTINATION include/kronos/math FILES_MATCHING PATTERN "*.h")
install(DIRECTORY core/mesh/ DESTINATION include/kronos/mesh FILES_MATCHING PATTERN "*.h")
install(DIRECTORY core/modifiers/ DESTINATION include/kronos/modifiers FILES_MATCHING PATTERN "*.h")
install(DIRECTORY vulkan/ DESTINATION include/kronos/vulkan FILES_MATCHING PATTERN "*.h")
install(DIRECTORY opengl/ DESTINATION include/kronos/opengl FILES_MATCHING PATTERN "*.h")
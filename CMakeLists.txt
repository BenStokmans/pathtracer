cmake_minimum_required(VERSION 3.31)
project(pathtracer LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add metal-cpp headers
include_directories(${PROJECT_SOURCE_DIR}/include/metal-cpp)

# Collect source files
file(GLOB CPP_SOURCES src/*.cpp src/*.h)
file(GLOB MM_SOURCES src/*.mm)

# Create executable
add_executable(pathtracer ${CPP_SOURCES} ${MM_SOURCES})

# Find required frameworks
find_library(METAL Metal REQUIRED)
find_library(COCOA Cocoa REQUIRED)
find_library(QUARTZCORE QuartzCore REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)

# Link frameworks
target_link_libraries(pathtracer PRIVATE
        ${METAL}
        ${COCOA}
        ${QUARTZCORE}
        ${FOUNDATION}
)

# shader directory
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

# list all of your .metal files here so CMake knows to re‐run if any of them change
set(SHADERS
        ${SHADER_DIR}/Types.metal
        ${SHADER_DIR}/RNG.metal
        ${SHADER_DIR}/BSDF.metal
        ${SHADER_DIR}/Intersection.metal
        ${SHADER_DIR}/Kernel.metal
)

# where to put the compiled library
set(METAL_LIBRARY_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/default.metallib)
set(AIR_FILE ${CMAKE_CURRENT_BINARY_DIR}/Kernel.air)

add_custom_command(
        OUTPUT ${METAL_LIBRARY_OUTPUT}
        COMMAND xcrun -sdk macosx metal
        -c
        -I ${SHADER_DIR}        # tell metal where to find your includes
        ${SHADER_DIR}/Kernel.metal
        -o ${AIR_FILE}
        COMMAND xcrun -sdk macosx metallib
        ${AIR_FILE}
        -o ${METAL_LIBRARY_OUTPUT}
        DEPENDS ${SHADERS}
        COMMENT "Compiling Metal shaders"
)

add_custom_target(CompileShaders DEPENDS ${METAL_LIBRARY_OUTPUT})
add_dependencies(pathtracer CompileShaders)

# Copy the metal library next to your executable
add_custom_command(TARGET pathtracer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${METAL_LIBRARY_OUTPUT}
        $<TARGET_FILE_DIR:pathtracer>/default.metallib
        COMMENT "Copying Metal library to output directory"
)
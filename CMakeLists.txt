cmake_minimum_required(VERSION 3.31)
project(pathtracer LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add metal-cpp headers
include_directories(${PROJECT_SOURCE_DIR}/include/metal-cpp)

# Include ImGui headers
include_directories(${PROJECT_SOURCE_DIR}/include/imgui)
include_directories(${PROJECT_SOURCE_DIR}/include/imgui/backends)

# Collect source files
file(GLOB CPP_SOURCES src/*.cpp src/*.h)
file(GLOB MM_SOURCES src/*.mm)

# Add ImGui source files
set(IMGUI_SOURCES
        ${PROJECT_SOURCE_DIR}/include/imgui/imgui.cpp
        ${PROJECT_SOURCE_DIR}/include/imgui/imgui_demo.cpp
        ${PROJECT_SOURCE_DIR}/include/imgui/imgui_draw.cpp
        ${PROJECT_SOURCE_DIR}/include/imgui/imgui_tables.cpp
        ${PROJECT_SOURCE_DIR}/include/imgui/imgui_widgets.cpp
        ${PROJECT_SOURCE_DIR}/include/imgui/backends/imgui_impl_metal.mm
)

# Create executable
add_executable(pathtracer ${CPP_SOURCES} ${MM_SOURCES} ${IMGUI_SOURCES})

# Set compile definitions for ImGui Metal C++ support
target_compile_definitions(pathtracer PRIVATE IMGUI_IMPL_METAL_CPP)

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

# Include all the *.metal files from the shaders directory
file(GLOB_RECURSE SHADERS ${SHADER_DIR}/*.metal)

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

# include objects in the binary
add_custom_command(TARGET pathtracer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:pathtracer>/assets
        COMMENT "Copying assets into runtime folder")
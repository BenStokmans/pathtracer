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

# Compile Metal shaders
set(METAL_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Shader.metal)
set(METAL_LIBRARY_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/default.metallib)

add_custom_command(
        OUTPUT ${METAL_LIBRARY_OUTPUT}
        COMMAND xcrun -sdk macosx metal -c ${METAL_SHADER_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/Shader.air
        COMMAND xcrun -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/Shader.air -o ${METAL_LIBRARY_OUTPUT}
        DEPENDS ${METAL_SHADER_SOURCE}
        COMMENT "Compiling Metal shaders"
)

add_custom_target(CompileShaders DEPENDS ${METAL_LIBRARY_OUTPUT})
add_dependencies(pathtracer CompileShaders)

# Copy the metal library to the output directory
add_custom_command(TARGET pathtracer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${METAL_LIBRARY_OUTPUT} $<TARGET_FILE_DIR:pathtracer>/default.metallib
        COMMENT "Copying Metal library to output directory"
)
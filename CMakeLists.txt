cmake_minimum_required(VERSION 3.25)
project(Vulkanized LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

find_package(glfw3 CONFIG REQUIRED)

find_package(Vulkan REQUIRED)

add_subdirectory(Libraries/glm)

add_executable(Vulkanized
        Project/main.cpp
        Project/Application.cpp
        Project/Application.h
        Project/Device.cpp
        Project/Device.h
        Project/Window.cpp
        Project/Window.h
        Project/Types.h
        Project/Logger.cpp
        Project/Logger.h
        Project/Pipeline.cpp
        Project/Pipeline.h
        Project/Swapchain.cpp
        Project/Swapchain.h
        Project/Definitions.h
        Project/Model.cpp
        Project/Model.h)

target_link_libraries(Vulkanized PRIVATE glfw Vulkan::Vulkan glm::glm)

# Add the path to your shader source files
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Assets/Shaders)

# Add the path where compiled shaders will be placed
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Assets/Shaders)

# Collect all shader source files
file(GLOB_RECURSE SHADER_SOURCES ${SHADER_SOURCE_DIR}/*.glsl)

# Set up custom command and custom target for shader compilation
add_custom_target(
        CompileShaders ALL
        COMMENT "Compiling shaders..."
        DEPENDS ${SHADER_SOURCES}
)

# Loop over each shader source file and add a custom command to compile it
foreach(SHADER_SOURCE ${SHADER_SOURCES})
    message(${SHADER_SOURCE})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WLE)
    set(SHADER_OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)
    message(${SHADER_OUTPUT})

    # Determine the shader stage based on the file extension
    if(${SHADER_SOURCE} MATCHES "\\.vert\\.glsl$")
        set(SHADER_STAGE "vert")
    elseif(${SHADER_SOURCE} MATCHES "\\.frag\\.glsl$")
        set(SHADER_STAGE "frag")
    elseif(${SHADER_SOURCE} MATCHES "\\.comp\\.glsl$")
        set(SHADER_STAGE "comp")
    else()
        message(FATAL_ERROR "Unknown shader stage for ${SHADER_SOURCE}")
    endif()

    add_custom_command(
            TARGET CompileShaders
            COMMAND glslc -fshader-stage=${SHADER_STAGE} ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling ${SHADER_NAME}..."
    )
endforeach()
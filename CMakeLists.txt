cmake_minimum_required(VERSION 3.28.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(minimal-metal-cpp VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)

## Build GLFW from source
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)

add_executable(minimal-metal-cpp
    src/main.cpp
    src/mtl_implementation.cpp
    src/mtl_engine.cpp
    dependencies/stb/stb/stb_image.cpp
    src/texture.cpp
)

# Metal Shader Compilation Functions
function(compile_metal_shader SHADER_SOURCE SHADER_OUTPUT)
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)

    # First compile .metal to .air (intermediate representation)
    set(AIR_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.air)
    add_custom_command(
        OUTPUT ${AIR_FILE}
        COMMAND xcrun -sdk macosx metal -c ${SHADER_SOURCE} -o ${AIR_FILE}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling Metal shader ${SHADER_SOURCE} to AIR"
        VERBATIM
    )

    # Then compile .air to .metallib (Metal library)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND xcrun -sdk macosx metallib ${AIR_FILE} -o ${SHADER_OUTPUT}
        DEPENDS ${AIR_FILE}
        COMMENT "Creating Metal library ${SHADER_OUTPUT}"
        VERBATIM
    )
endfunction()

# Function to add multiple shaders and create a combined library
function(add_metal_library TARGET_NAME EXECUTABLE_TARGET)
    set(METAL_SOURCES ${ARGN})
    set(AIR_FILES "")
    set(METALLIB_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.metallib)

    # Compile each .metal file to .air
    foreach(METAL_SOURCE ${METAL_SOURCES})
        get_filename_component(SHADER_NAME ${METAL_SOURCE} NAME_WE)
        set(AIR_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.air)

        add_custom_command(
            OUTPUT ${AIR_FILE}
            COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_SOURCE_DIR}/${METAL_SOURCE} -o ${AIR_FILE} -I${CMAKE_CURRENT_SOURCE_DIR}/src
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${METAL_SOURCE}
            COMMENT "Compiling ${METAL_SOURCE} to AIR"
            VERBATIM
        )

        list(APPEND AIR_FILES ${AIR_FILE})
    endforeach()

    # Combine all .air files into a single .metallib
    add_custom_command(
        OUTPUT ${METALLIB_FILE}
        COMMAND xcrun -sdk macosx metallib ${AIR_FILES} -o ${METALLIB_FILE}
        DEPENDS ${AIR_FILES}
        COMMENT "Creating combined Metal library ${TARGET_NAME}.metallib"
        VERBATIM
    )

    # Create a custom target for the metal library
    add_custom_target(${TARGET_NAME}_shaders ALL DEPENDS ${METALLIB_FILE})

    # Make sure the metallib gets copied to the right place
    add_custom_command(TARGET ${TARGET_NAME}_shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${METALLIB_FILE} $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>/${TARGET_NAME}.metallib
        COMMENT "Copying ${TARGET_NAME}.metallib to output directory"
    )
endfunction()

# Add Metal shader compilation
add_metal_library(shaders minimal-metal-cpp
    src/shaders/triangle.metal
    src/shaders/square.metal
    # Add more .metal files here as needed
)

# Make sure shaders are built before the main target
add_dependencies(minimal-metal-cpp shaders_shaders)

# # Copy metal files to build WORKING_DIRECTORY
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/triangle.metal
#     ${CMAKE_CURRENT_BINARY_DIR}/triangle.metal COPYONLY)

# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/square.metal
#     ${CMAKE_CURRENT_BINARY_DIR}/square.metal COPYONLY)

# Copy assets folder to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Include Metal Dependencies and Metal Extensions (provided by Apple in 2021)
target_include_directories(minimal-metal-cpp
    PRIVATE
    # Apple's Metal headers for C++
    dependencies/metal-cpp
    # Apple's AppKit and MetalKit headers for C++ with custom extensions
    dependencies/metal-cpp-extensions
    # STB Image loading library for loading textures
    dependencies/stb
    # AAPLMathUtilities
    dependencies/AAPLMathUtilities
)

target_sources(minimal-metal-cpp
    PRIVATE
    dependencies/AAPLMathUtilities/AAPLMathUtilities.cpp
)

# Link dependencies
target_link_libraries(minimal-metal-cpp
    PRIVATE
    glfw
    "-framework Metal"
    "-framework Foundation"
    "-framework MetalFX"
    "-framework QuartzCore"
    "-framework AppKit"
    "-framework MetalKit"
)


# Custom target to build and run
add_custom_target(run
    COMMAND minimal-metal-cpp
    DEPENDS minimal-metal-cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

cmake_minimum_required(VERSION 3.16)
project(Kostur)

set(CMAKE_CXX_STANDARD 17)

# --- GLAD library ---
add_library(glad STATIC packages/glad/src/glad.c)
target_include_directories(glad PUBLIC packages/glad/include)

# --- Main executable ---
file(GLOB SRC_FILES src/*.cpp)
add_executable(Kostur ${SRC_FILES}
        Header/model.hpp
        Header/mesh.hpp
        Header/shader.hpp
        Header/ground.hpp
        src/ground.cpp
        src/ground.h)

# --- Common includes ---
target_include_directories(Kostur PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
        ${CMAKE_CURRENT_SOURCE_DIR}/packages/glad/include
)

# --- Windows configuration ---
if(WIN32)
    message(STATUS "Configuring for Windows...")

    # Option 1: Use system-installed GLFW (if installed via vcpkg)
    # find_package(glfw3 CONFIG REQUIRED)

    # Option 2: Build GLFW from source (recommended for portability)
    include(FetchContent)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.3.8) # Using 3.3.8 for stability
    FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
    FetchContent_Declare(assimp GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG v5.2.5)
    
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw glm assimp)

    # Link libraries
    target_link_libraries(Kostur
            glad
            glfw
            glm::glm
            assimp::assimp
            opengl32
            gdi32
    )

    # --- Linux configuration ---
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux...")

    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)

    # Link libraries
    target_link_libraries(Kostur
            glad
            glfw
            ${OPENGL_LIBRARIES}
    )

    # Add pthread for Linux
    find_package(Threads REQUIRED)
    target_link_libraries(Kostur Threads::Threads)

    # --- macOS configuration ---
elseif(APPLE)
    message(STATUS "Configuring for macOS...")

    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)

    target_link_libraries(Kostur
            glad
            glfw
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()

# --- Copy resources ---
if(EXISTS ${CMAKE_SOURCE_DIR}/resources/shaders)
    add_custom_command(TARGET Kostur POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources/shaders
            ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders
    )
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(TARGET Kostur POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources
    )
endif()
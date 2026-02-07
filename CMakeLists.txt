cmake_minimum_required(VERSION 3.16)
project(Kostur LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------
# Dependencies (provided by vcpkg)
# vcpkg integrate install MUST be done beforehand
# -------------------------------------------------

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# -------------------------------------------------
# Main executable
# -------------------------------------------------

add_executable(Kostur
        src/main.cpp
        src/ground.cpp

        Header/model.h
        Header/mesh.h
        Header/shader.h
        Header/ground.h
        src/Util.cpp
)

target_include_directories(Kostur PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
)

# -------------------------------------------------
# Link libraries
# -------------------------------------------------

target_link_libraries(Kostur
        PRIVATE
        glad::glad
        glfw
        glm::glm
        assimp::assimp
        opengl32
        gdi32
)

# -------------------------------------------------
# Copy resources after build
# -------------------------------------------------

if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(
            TARGET Kostur POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:Kostur>/resources
    )
endif()

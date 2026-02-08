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
find_package(GLEW CONFIG REQUIRED)

add_executable(Kostur
        src/Main.cpp
        src/ground.cpp
        src/Util.cpp
        src/Util2D.cpp
        src/Util3D.cpp

        Header/model.h
        Header/mesh.h
        Header/shader.h
        Header/ground.h
        Header/Util2D.h
        Header/Util3D.h
        src/Util2D.cpp
)

target_include_directories(Kostur PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
)

target_link_libraries(Kostur
        PRIVATE
        GLEW::GLEW
        glfw
        glm::glm
        assimp::assimp
        opengl32
        gdi32
)

if (EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(
            TARGET Kostur POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:Kostur>/resources
    )
endif ()

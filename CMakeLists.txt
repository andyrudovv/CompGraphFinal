cmake_minimum_required(VERSION 3.16)
project(LaneRunner LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
# freeglut: vcpkg ships a CMake config (FreeGLUT::freeglut), Linux/macOS packages use FindGLUT.
find_package(FreeGLUT CONFIG QUIET)
if(NOT FreeGLUT_FOUND)
    find_package(GLUT REQUIRED)
endif()
find_package(glm CONFIG REQUIRED)
add_executable(laneRunner
    engine/window.cpp engine/shader.cpp engine/mesh.cpp engine/renderer.cpp
    engine/texgen.cpp engine/texture.cpp engine/hud.cpp engine/objloader.cpp
    game/game.cpp game/main.cpp)
if(TARGET FreeGLUT::freeglut)
    target_link_libraries(laneRunner PRIVATE FreeGLUT::freeglut)
else()
    target_link_libraries(laneRunner PRIVATE GLUT::GLUT)
endif()
target_link_libraries(laneRunner PRIVATE OpenGL::GL GLEW::GLEW glm::glm)
if(MSVC)
    target_compile_options(laneRunner PRIVATE /W4)
else()
    target_compile_options(laneRunner PRIVATE -Wall -Wextra)
endif()
# Assets in the build tree also work when launched from another directory.
add_custom_target(laneRunnerAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_CURRENT_BINARY_DIR}/shaders")
add_dependencies(laneRunner laneRunnerAssets)
target_compile_definitions(laneRunner PRIVATE
    LANE_RUNNER_ASSET_DIR="${CMAKE_CURRENT_BINARY_DIR}")

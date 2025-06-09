cmake_minimum_required(VERSION 3.16)
project(SolarSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

message(STATUS "[Config] Project source directory: ${SRC_DIR}")
message(STATUS "[Config] Third-party directory: ${THIRD_PARTY_DIR}")

include_directories(
    ${SRC_DIR}
    ${SRC_DIR}/core
    ${SRC_DIR}/objects
    ${SRC_DIR}/ui
    ${THIRD_PARTY_DIR}
    ${THIRD_PARTY_DIR}/imgui
)

message(STATUS "[Includes] Configured include directories:")
foreach(dir
    ${SRC_DIR}
    ${SRC_DIR}/core
    ${SRC_DIR}/objects
    ${THIRD_PARTY_DIR}/glad
    ${THIRD_PARTY_DIR}/GLFW
    ${THIRD_PARTY_DIR}/glm
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/imgui/backends
    ${THIRD_PARTY_DIR}/KHR
)
    message(STATUS "  - ${dir}")
endforeach()

file(GLOB_RECURSE SOURCES
    ${SRC_DIR}/*.cpp
    ${THIRD_PARTY_DIR}/glad/glad.c
    ${THIRD_PARTY_DIR}/imgui/*.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

message(STATUS "[Sources] Collected source files:")
foreach(source ${SOURCES})
    message(STATUS "  - ${source}")
endforeach()

add_executable(SolarSystemGL ${SOURCES} "src/core/Constants.h")

target_link_libraries(SolarSystemGL
    ${THIRD_PARTY_DIR}/glfw/lib-vc2022/glfw3.lib
    opengl32
)

# Copy shaders to build directory
add_custom_command(TARGET SolarSystemGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SHADER_DIR}" "${SHADER_OUTPUT_DIR}"
    COMMENT "Copying shaders to build directory..."
)

message(STATUS "[Linking] Linked libraries:")
message(STATUS "  - glfw3.lib (VC2022)")
message(STATUS "  - opengl32")

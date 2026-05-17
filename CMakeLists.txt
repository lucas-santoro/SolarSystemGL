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

# Explicit source list — preferred over file(GLOB_RECURSE). Adding a new .cpp
# now requires editing this list, but you no longer need to manually rerun
# `cmake -S . -B build` whenever you create a file (the old glob behavior was
# the source of "missing symbol" link errors when files were added without
# reconfigure).
set(PROJECT_SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/core/Application.cpp
    ${SRC_DIR}/core/Camera.cpp
    ${SRC_DIR}/core/Grid.cpp
    ${SRC_DIR}/core/SaveLoad.cpp
    ${SRC_DIR}/core/Shader.cpp
    ${SRC_DIR}/core/Window.cpp
    ${SRC_DIR}/objects/CelestialBody.cpp
    ${SRC_DIR}/objects/PlanetMesh.cpp
    ${SRC_DIR}/physics/PhysicsSystem.cpp
    ${SRC_DIR}/ui/StartMenu.cpp
    ${SRC_DIR}/ui/Toast.cpp
    ${SRC_DIR}/ui/UIManager.cpp
)

set(THIRD_PARTY_SOURCES
    ${THIRD_PARTY_DIR}/glad/glad.c
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

message(STATUS "[Sources] Project files:")
foreach(source ${PROJECT_SOURCES})
    message(STATUS "  - ${source}")
endforeach()

add_executable(SolarSystemGL ${PROJECT_SOURCES} ${THIRD_PARTY_SOURCES})

# Platform-conditional linker: keep the vendored GLFW on Windows (tested), use
# find_package on Linux / macOS where system or vcpkg-installed glfw3 is common.
if (WIN32)
    target_link_libraries(SolarSystemGL
        ${THIRD_PARTY_DIR}/glfw/lib-vc2022/glfw3.lib
        opengl32
    )
    message(STATUS "[Linking] Windows: vendored glfw3.lib (VC2022) + opengl32")
else()
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(SolarSystemGL PRIVATE glfw OpenGL::GL)
    message(STATUS "[Linking] Non-Windows: find_package(glfw3) + OpenGL::GL")
endif()

# Copy shaders next to the executable so they resolve regardless of CWD
# (multi-config generators put the exe under Release/, Debug/, etc.).
add_custom_command(TARGET SolarSystemGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SHADER_DIR}" "$<TARGET_FILE_DIR:SolarSystemGL>/shaders"
    COMMENT "Copying shaders next to executable..."
)

# Copy preset save-files next to the executable, same rationale as shaders.
add_custom_command(TARGET SolarSystemGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/presets" "$<TARGET_FILE_DIR:SolarSystemGL>/presets"
    COMMENT "Copying presets next to executable..."
)

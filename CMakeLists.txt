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
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# --- Embed GLSL shaders into the executable --------------------------------
# Each .glsl is read at configure time and stamped into EmbeddedShaders.h via
# configure_file. CMAKE_CONFIGURE_DEPENDS forces a reconfigure (and thus a
# regenerated header) whenever any shader source changes.
set(SHADER_SOURCES
    "${SHADER_DIR}/VertexShader.glsl"
    "${SHADER_DIR}/FragmentShader.glsl"
    "${SHADER_DIR}/GridVertexShader.glsl"
    "${SHADER_DIR}/GridFragmentShader.glsl"
    "${SHADER_DIR}/TrailVertexShader.glsl"
    "${SHADER_DIR}/TrailFragmentShader.glsl"
    "${SHADER_DIR}/SkyVertexShader.glsl"
    "${SHADER_DIR}/SkyFragmentShader.glsl"
    "${SHADER_DIR}/RingVertexShader.glsl"
    "${SHADER_DIR}/RingFragmentShader.glsl"
    "${SHADER_DIR}/FullscreenQuadVertexShader.glsl"
    "${SHADER_DIR}/BloomBlurFragmentShader.glsl"
    "${SHADER_DIR}/BloomCompositeFragmentShader.glsl"
    "${SHADER_DIR}/AtmosphereVertexShader.glsl"
    "${SHADER_DIR}/AtmosphereFragmentShader.glsl"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADER_SOURCES})

file(READ "${SHADER_DIR}/VertexShader.glsl"                  VERTEX_SHADER_SOURCE)
file(READ "${SHADER_DIR}/FragmentShader.glsl"                FRAGMENT_SHADER_SOURCE)
file(READ "${SHADER_DIR}/GridVertexShader.glsl"              GRID_VERTEX_SHADER_SOURCE)
file(READ "${SHADER_DIR}/GridFragmentShader.glsl"            GRID_FRAGMENT_SHADER_SOURCE)
file(READ "${SHADER_DIR}/TrailVertexShader.glsl"             TRAIL_VERTEX_SHADER_SOURCE)
file(READ "${SHADER_DIR}/TrailFragmentShader.glsl"           TRAIL_FRAGMENT_SHADER_SOURCE)
file(READ "${SHADER_DIR}/SkyVertexShader.glsl"               SKY_VERTEX_SHADER_SOURCE)
file(READ "${SHADER_DIR}/SkyFragmentShader.glsl"             SKY_FRAGMENT_SHADER_SOURCE)
file(READ "${SHADER_DIR}/RingVertexShader.glsl"              RING_VERTEX_SHADER_SOURCE)
file(READ "${SHADER_DIR}/RingFragmentShader.glsl"            RING_FRAGMENT_SHADER_SOURCE)
file(READ "${SHADER_DIR}/FullscreenQuadVertexShader.glsl"    FULLSCREEN_QUAD_VERTEX_SHADER_SOURCE)
file(READ "${SHADER_DIR}/BloomBlurFragmentShader.glsl"       BLOOM_BLUR_FRAGMENT_SHADER_SOURCE)
file(READ "${SHADER_DIR}/BloomCompositeFragmentShader.glsl"  BLOOM_COMPOSITE_FRAGMENT_SHADER_SOURCE)
file(READ "${SHADER_DIR}/AtmosphereVertexShader.glsl"        ATMOSPHERE_VERTEX_SHADER_SOURCE)
file(READ "${SHADER_DIR}/AtmosphereFragmentShader.glsl"      ATMOSPHERE_FRAGMENT_SHADER_SOURCE)

configure_file(
    "${SRC_DIR}/core/EmbeddedShaders.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedShaders.h"
    @ONLY
)
message(STATUS "[Shaders] Embedded ${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedShaders.h")

# Explicit source list — preferred over file(GLOB_RECURSE). Adding a new .cpp
# now requires editing this list, but you no longer need to manually rerun
# `cmake -S . -B build` whenever you create a file (the old glob behavior was
# the source of "missing symbol" link errors when files were added without
# reconfigure).
set(PROJECT_SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/core/Application.cpp
    ${SRC_DIR}/core/Camera.cpp
    ${SRC_DIR}/core/Framebuffer.cpp
    ${SRC_DIR}/core/Grid.cpp
    ${SRC_DIR}/core/SaveLoad.cpp
    ${SRC_DIR}/core/Screenshot.cpp
    ${SRC_DIR}/core/Shader.cpp
    ${SRC_DIR}/core/WebPersistence.cpp
    ${SRC_DIR}/core/Window.cpp
    ${SRC_DIR}/objects/CelestialBody.cpp
    ${SRC_DIR}/objects/PlanetMesh.cpp
    ${SRC_DIR}/physics/Lagrange.cpp
    ${SRC_DIR}/physics/PathPredictor.cpp
    ${SRC_DIR}/physics/PhysicsSystem.cpp
    ${SRC_DIR}/ui/Actionbar.cpp
    ${SRC_DIR}/ui/AddPlanetModal.cpp
    ${SRC_DIR}/ui/SaveLoadModal.cpp
    ${SRC_DIR}/ui/SettingsModal.cpp
    ${SRC_DIR}/ui/StartMenu.cpp
    ${SRC_DIR}/ui/Toast.cpp
    ${SRC_DIR}/ui/Tutorial.cpp
    ${SRC_DIR}/ui/UIManager.cpp
)

set(THIRD_PARTY_SOURCES
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
# glad.c only feeds the desktop OpenGL function-pointer loader. Emscripten
# talks to WebGL2 directly so this file is excluded from the wasm build.
if (NOT EMSCRIPTEN)
    list(APPEND THIRD_PARTY_SOURCES ${THIRD_PARTY_DIR}/glad/glad.c)
endif()

message(STATUS "[Sources] Project files:")
foreach(source ${PROJECT_SOURCES})
    message(STATUS "  - ${source}")
endforeach()

add_executable(SolarSystemGL ${PROJECT_SOURCES} ${THIRD_PARTY_SOURCES})

# Platform-conditional linker:
# - Emscripten: WebGL2 + emscripten's bundled GLFW3 port + virtual FS preloads.
# - Windows: vendored GLFW3 .lib (VC2022 build).
# - Linux / macOS: find_package(glfw3) + system OpenGL.
if (EMSCRIPTEN)
    # Emscripten supplies a GLFW3-compatible glue layer via -sUSE_GLFW=3
    # and exposes WebGL2 as OpenGL ES 3 with -sUSE_WEBGL2=1 / -sFULL_ES3=1.
    # No find_package, no vendored .lib — the linker is em++.
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    target_compile_options(SolarSystemGL PRIVATE
        "-sUSE_GLFW=3"
    )
    target_link_options(SolarSystemGL PRIVATE
        "-sUSE_GLFW=3"
        "-sUSE_WEBGL2=1"
        "-sFULL_ES3=1"
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=64MB"
        "-sSTACK_SIZE=4MB"
        # IDBFS: persistent filesystem backed by IndexedDB. Mounted at /user
        # in WebPersistence.cpp so the user can keep saves across reloads.
        "-lidbfs.js"
        # ASYNCIFY lets EM_ASYNC_JS suspend the C++ stack while awaiting
        # FS.syncfs() and the file picker promise. Adds ~5 KB to the wasm.
        "-sASYNCIFY"
        # Exports needed for the WebPersistence file picker JS to allocate
        # and write a UTF-8 payload back into the wasm heap.
        "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8','lengthBytesUTF8','HEAPU8']"
        "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free']"
        # Preload the runtime data files (presets + Inter font) into the
        # virtual filesystem at "/presets" and "/fonts" so std::ifstream
        # paths in the C++ code resolve identically to the native build.
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/presets@/presets"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts@/fonts"
        # Custom HTML shell with a fullscreen canvas, dark background, no
        # emscripten branding, and a loading overlay that follows the
        # .data download progress + the runtime-init handshake.
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
    )
    # SOLARSYSTEM_BUILD_WEB         — our own switch for code branches.
    # IMGUI_IMPL_OPENGL_ES3         — tells imgui_impl_opengl3.cpp to emit
    #                                 OpenGL ES 3 calls (and the right shader
    #                                 preamble) instead of desktop GL.
    target_compile_definitions(SolarSystemGL PRIVATE
        SOLARSYSTEM_BUILD_WEB=1
        IMGUI_IMPL_OPENGL_ES3=1
    )
    message(STATUS "[Linking] Emscripten: -sUSE_GLFW=3 -sUSE_WEBGL2=1 + preload-file presets/ + fonts/")
elseif (WIN32)
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

# Shaders are now embedded into the executable via EmbeddedShaders.h (see
# the configure_file invocation above) — no runtime shader directory is
# needed alongside the binary.

# Preset / font copying only matters for native builds — emscripten
# preloads them straight into the virtual FS at link time (see
# --preload-file flags above).
if (NOT EMSCRIPTEN)
    # Copy preset save-files next to the executable so loading defaults works
    # regardless of CWD (multi-config generators put the exe under Release/,
    # Debug/, etc.).
    add_custom_command(TARGET SolarSystemGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/presets" "$<TARGET_FILE_DIR:SolarSystemGL>/presets"
        COMMENT "Copying presets next to executable..."
    )

    # Copy the bundled TTF font next to the executable so ImGui can load it at
    # startup. Same rationale as the presets POST_BUILD copy.
    add_custom_command(TARGET SolarSystemGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts" "$<TARGET_FILE_DIR:SolarSystemGL>/fonts"
        COMMENT "Copying fonts next to executable..."
    )
endif()

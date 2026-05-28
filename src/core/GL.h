#pragma once

/**
 * @file GL.h
 * @brief OpenGL / WebGL2 header umbrella.
 *
 * Native builds (Windows / Linux / macOS) use glad to load the desktop
 * OpenGL 3.3 core function pointers at runtime — every translation unit
 * that touches GL pulls `glad/glad.h` through this header.
 *
 * Emscripten builds target WebGL2 directly via `<GLES3/gl3.h>`; the JS
 * glue layer resolves the function table, so no loader is needed. The
 * `SOLARSYSTEM_BUILD_WEB` define is set by CMakeLists for the wasm path.
 */

#ifdef SOLARSYSTEM_BUILD_WEB
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#else
#include <glad/glad.h>
#endif

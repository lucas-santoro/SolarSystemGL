#pragma once

#include <string>

/**
 * @file Screenshot.h
 * @brief Framebuffer → 24-bit BMP exporter.
 */

/**
 * @brief Read the current OpenGL framebuffer and write it to a 24-bit BMP file.
 *
 * Writes BGR pixel data with row padding to 4-byte alignment, the file layout
 * native BMP expects. The caller is responsible for ensuring a GL context is
 * current and that @p path's parent directory exists.
 *
 * @param path   Filesystem path of the .bmp file to create (or overwrite).
 * @param width  Framebuffer width in pixels.
 * @param height Framebuffer height in pixels.
 * @return       `true` on success; `false` if the file cannot be opened.
 */
bool captureFramebufferToBMP(const std::string& path, int width, int height);

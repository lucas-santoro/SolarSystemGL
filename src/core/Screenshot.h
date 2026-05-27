#pragma once

#include <string>

/**
 * @file Screenshot.h
 * @brief Framebuffer → 24-bit BMP exporter.
 *
 * Two flavors:
 *  - `encodeFramebufferAsBMP` — pure: reads the framebuffer and returns the
 *    raw BMP byte stream. Useful for handing the bytes to a browser download
 *    or any other sink that isn't a filesystem path.
 *  - `captureFramebufferToBMP` — convenience: encode + write to disk.
 */

/**
 * @brief Read the current OpenGL framebuffer and produce an in-memory 24-bit
 *        BMP byte stream (file header + DIB header + BGR pixel data, padded
 *        to 4-byte rows). The caller is responsible for ensuring a GL context
 *        is current.
 *
 * @param width  Framebuffer width in pixels.
 * @param height Framebuffer height in pixels.
 * @return       BMP bytes, or empty string if @p width / @p height are non-positive.
 */
std::string encodeFramebufferAsBMP(int width, int height);

/**
 * @brief Read the current OpenGL framebuffer and write it to a 24-bit BMP file.
 *
 * The caller is responsible for ensuring a GL context is current and that
 * @p path's parent directory exists.
 *
 * @param path   Filesystem path of the .bmp file to create (or overwrite).
 * @param width  Framebuffer width in pixels.
 * @param height Framebuffer height in pixels.
 * @return       `true` on success; `false` if the file cannot be opened.
 */
bool captureFramebufferToBMP(const std::string& path, int width, int height);

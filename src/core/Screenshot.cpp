#include "Screenshot.h"

#include "core/GL.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>

namespace
{
void writeUint16LE(std::ostream& stream, uint16_t value)
{
    const std::array<char, 2> bytes = {static_cast<char>(value & 0xFF),
                                       static_cast<char>((value >> 8) & 0xFF)};
    stream.write(bytes.data(), bytes.size());
}

void writeUint32LE(std::ostream& stream, uint32_t value)
{
    const std::array<char, 4> bytes = {
        static_cast<char>(value & 0xFF), static_cast<char>((value >> 8) & 0xFF),
        static_cast<char>((value >> 16) & 0xFF), static_cast<char>((value >> 24) & 0xFF)};
    stream.write(bytes.data(), bytes.size());
}

void writeInt32LE(std::ostream& stream, int32_t value)
{
    writeUint32LE(stream, static_cast<uint32_t>(value));
}

/// Read the framebuffer and stream BMP bytes into @p out.
void writeBmpStream(std::ostream& out, int width, int height)
{
    const int rowStride           = (width * 3 + 3) & ~3; // 4-byte aligned per BMP spec
    const int pixelDataSize       = rowStride * height;
    constexpr int kFileHeaderSize = 14;
    constexpr int kInfoHeaderSize = 40;
    const int fileSize            = kFileHeaderSize + kInfoHeaderSize + pixelDataSize;

    // OpenGL hands us tightly packed RGB pixels, bottom-to-top — same row
    // order BMP wants. We only need to swap R↔B and pad rows.
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // ---- BMP file header (14 bytes) ----
    out.put('B').put('M');
    writeUint32LE(out, static_cast<uint32_t>(fileSize));
    writeUint16LE(out, 0);                                 // reserved
    writeUint16LE(out, 0);                                 // reserved
    writeUint32LE(out, kFileHeaderSize + kInfoHeaderSize); // pixel data offset

    // ---- DIB info header (40 bytes, BITMAPINFOHEADER) ----
    writeUint32LE(out, kInfoHeaderSize);
    writeInt32LE(out, width);
    writeInt32LE(out, height);
    writeUint16LE(out, 1);  // planes
    writeUint16LE(out, 24); // bits per pixel
    writeUint32LE(out, 0);  // BI_RGB (no compression)
    writeUint32LE(out, static_cast<uint32_t>(pixelDataSize));
    writeInt32LE(out, 0);  // x pixels per meter
    writeInt32LE(out, 0);  // y pixels per meter
    writeUint32LE(out, 0); // colors used
    writeUint32LE(out, 0); // colors important

    // ---- Pixel data ----
    std::vector<char> row(static_cast<size_t>(rowStride), 0);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const size_t srcIndex = (static_cast<size_t>(y) * width + x) * 3;
            row[x * 3 + 0]        = static_cast<char>(pixels[srcIndex + 2]); // B
            row[x * 3 + 1]        = static_cast<char>(pixels[srcIndex + 1]); // G
            row[x * 3 + 2]        = static_cast<char>(pixels[srcIndex + 0]); // R
        }
        out.write(row.data(), rowStride);
    }
}
} // namespace

std::string encodeFramebufferAsBMP(int width, int height)
{
    if (width <= 0 || height <= 0)
        return {};

    std::ostringstream ss(std::ios::binary);
    writeBmpStream(ss, width, height);
    return ss.str();
}

bool captureFramebufferToBMP(const std::string& path, int width, int height)
{
    if (width <= 0 || height <= 0)
        return false;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
        return false;

    writeBmpStream(file, width, height);
    return static_cast<bool>(file);
}

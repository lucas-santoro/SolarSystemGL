# Build Guide

This guide provides comprehensive instructions for building SolarSystemGL on different platforms and configurations.

## 📋 Prerequisites

### Required Software

#### Windows
- **Visual Studio 2022** (Community, Professional, or Enterprise)
  - Include "Desktop development with C++" workload
  - MSVC v143 compiler toolset
  - Windows 10/11 SDK
- **CMake 3.16+** ([Download](https://cmake.org/download/))
- **Git** ([Download](https://git-scm.com/download/win))

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake via Homebrew
brew install cmake
```

### Hardware Requirements
- **OpenGL 3.3+** compatible graphics card
- **2GB RAM** minimum, 4GB recommended
- **500MB** free disk space

## 🛠️ Building from Source

### Method 1: Visual Studio (Windows)

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/SolarSystemGL.git
   cd SolarSystemGL
   ```

2. **Generate Visual Studio solution**
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

3. **Open in Visual Studio**
   - Open `SolarSystemGL.sln` in Visual Studio
   - Set build configuration to `Release`
   - Build → Build Solution (Ctrl+Shift+B)

4. **Run the application**
   - Set `SolarSystemGL` as startup project
   - Press F5 to run with debugging, or Ctrl+F5 without debugging

### Method 2: Command Line (All Platforms)

1. **Clone and navigate**
   ```bash
   git clone https://github.com/yourusername/SolarSystemGL.git
   cd SolarSystemGL
   ```

2. **Configure build**
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build project**
   ```bash
   # Windows (Visual Studio)
   cmake --build . --config Release

   # Linux/macOS
   make -j$(nproc)
   ```

4. **Run executable**
   ```bash
   # Windows
   ./Release/SolarSystemGL.exe

   # Linux/macOS
   ./SolarSystemGL
   ```

### Method 3: CMake GUI (Windows)

1. **Open CMake GUI**
   - Set source directory to project root
   - Set build directory to `build/` folder

2. **Configure**
   - Click "Configure"
   - Select generator (Visual Studio 17 2022)
   - Choose platform (x64)

3. **Generate**
   - Click "Generate"
   - Click "Open Project" to launch Visual Studio

## ⚙️ Build Configuration Options

### CMake Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build configuration | `Release` |
| `CMAKE_CXX_STANDARD` | C++ standard version | `17` |
| `BUILD_SHARED_LIBS` | Build shared libraries | `OFF` |

### Custom Configuration
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20
```

## 🔧 Troubleshooting

### Common Issues

#### OpenGL Not Found
**Error**: `OpenGL not found`
**Solution**: 
- Windows: Install latest graphics drivers
- Linux: `sudo apt install mesa-common-dev`
- macOS: Update Xcode

#### GLFW Not Found
**Error**: `Could not find GLFW`
**Solution**:
- The project includes GLFW in `third_party/`
- Ensure all files were cloned correctly
- Check CMakeLists.txt paths

#### Visual Studio Build Errors
**Error**: `MSB8066: Custom build for '...' exited with code 1`
**Solution**:
- Clean solution (Build → Clean Solution)
- Rebuild (Build → Rebuild Solution)
- Check Windows SDK version

#### CMake Configuration Failed
**Error**: `CMake Error: Could not find CMAKE_CXX_COMPILER`
**Solution**:
- Install Visual Studio Build Tools
- Ensure compiler is in PATH
- Use Visual Studio Developer Command Prompt

### Performance Issues

#### Low FPS
- Update graphics drivers
- Reduce grid resolution in `Grid.cpp`
- Build in Release mode, not Debug

#### High Memory Usage
- Reduce planet subdivision levels
- Lower grid resolution
- Check for memory leaks in Debug mode

## 📦 Dependencies

### Included Libraries (third_party/)
- **GLFW 3.3**: Window management and input
- **GLAD**: OpenGL loader
- **GLM**: Mathematics library
- **ImGui**: Immediate mode GUI
- **KHR**: Khronos headers

### System Dependencies
- **OpenGL 3.3+**: Graphics API
- **Windows API**: Platform-specific features (Windows only)

## 🧪 Testing the Build

### Verification Steps

1. **Launch Application**
   - Window should open with title "SolarSystemGL"
   - Should display 3D solar system with planets

2. **Test Controls**
   - WASD keys should move camera
   - Mouse should control camera rotation
   - Scroll wheel should zoom in/out

3. **Test UI**
   - Menu bar should be visible
   - Clicking planets should show info panel
   - FPS counter should be visible

### Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

Debug builds include:
- Debug symbols for debugging
- Runtime error checking
- Memory leak detection
- Performance profiling hooks

## 🚀 Optimization

### Release Build Optimizations
- Compiler optimizations (-O2/-O3)
- Link-time optimization
- Dead code elimination
- Inlining optimizations

### Custom Optimizations
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

## 📁 Build Output Structure

```
build/
├── CMakeCache.txt
├── CMakeFiles/
├── Release/              # Windows
│   ├── SolarSystemGL.exe
│   └── shaders/
├── Debug/                # Windows
│   ├── SolarSystemGL.exe
│   └── shaders/
├── SolarSystemGL         # Linux/macOS
└── shaders/              # Copied shader files
```

## 🔄 Continuous Integration

### GitHub Actions Example
```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    - name: Build
      run: cmake --build build --config Release
    
    - name: Test
      run: ./build/Release/SolarSystemGL.exe --test
```

## 📞 Support

If you encounter build issues:

1. **Check Prerequisites**: Ensure all required software is installed
2. **Clean Build**: Delete `build/` folder and rebuild
3. **Check Logs**: Review CMake and compiler output
4. **Update Dependencies**: Ensure latest versions
5. **Create Issue**: Report bugs with full error logs

### Getting Help
- **GitHub Issues**: Report build problems
- **Discord**: Join our community for real-time help
- **Email**: Contact developers directly

---

**Happy Building! 🚀** 
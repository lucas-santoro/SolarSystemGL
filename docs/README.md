# SolarSystemGL

A real-time 3D solar system simulation built with OpenGL, featuring accurate planetary physics, interactive controls, and stunning visual effects.

![SolarSystemGL](https://img.shields.io/badge/OpenGL-3.3-blue) ![C++](https://img.shields.io/badge/C%2B%2B-17-green) ![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey) ![License](https://img.shields.io/badge/License-MIT-yellow)

## 🌟 Features

### 🌍 Realistic Physics Simulation
- **Newtonian Gravity**: Accurate gravitational interactions between all celestial bodies
- **Real Astronomical Data**: Authentic masses, densities, and orbital distances
- **Time Acceleration**: Simulate 10 days per second for observable orbital motion
- **N-Body Problem**: Dynamic gravitational forces affecting all planets simultaneously

### 🎮 Interactive 3D Visualization
- **Modern OpenGL Rendering**: Hardware-accelerated 3D graphics with custom shaders
- **Procedural Planet Generation**: Spherical planets created using icosahedron subdivision
- **Spacetime Grid**: Visual representation of gravitational field distortion
- **Smooth Camera Controls**: Free-roam camera with WASD movement and mouse look

### 🖱️ Advanced User Interface
- **Ray-Cast Planet Selection**: Click directly on planets for precise interaction
- **Real-Time Property Editing**: Modify planet mass, density, position, and velocity
- **Smart Camera Focusing**: Automatic smooth camera movement to selected planets
- **Responsive UI**: ImGui-based interface with professional styling

### 🔧 Technical Excellence
- **Modular Architecture**: Clean separation of concerns with organized code structure
- **Efficient Rendering**: Optimized OpenGL pipeline with proper resource management
- **Cross-Platform Ready**: CMake build system for easy compilation
- **Extensible Design**: Easy to add new features and celestial bodies

## 🚀 Quick Start

### Prerequisites
- **Visual Studio 2022** (or 2019 with MSVC)
- **CMake 3.16+**
- **OpenGL 3.3+** compatible graphics card

### Building the Project

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/SolarSystemGL.git
   cd SolarSystemGL
   ```

2. **Create build directory**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**
   ```bash
   cmake ..
   ```

4. **Build the project**
   ```bash
   cmake --build . --config Release
   ```

5. **Run the simulation**
   ```bash
   ./Release/SolarSystemGL.exe
   ```

## 🎯 Controls

### Camera Movement
- **WASD**: Move camera forward/backward/left/right
- **Mouse**: Look around (right-click and drag)
- **Scroll Wheel**: Zoom in/out
- **Left Click on Planet**: Focus camera on selected planet

### User Interface
- **Main Menu**: Access planet list, camera controls, and settings
- **Planet Info Panel**: Edit selected planet properties
- **Solar System Panel**: View FPS and add new planets
- **Hover Labels**: Planet names appear when hovering

## 📊 Technical Specifications

### Physics Engine
- **Gravitational Constant**: 6.67430×10⁻¹¹ m³/kg/s²
- **Time Scale**: 860,400× real-time (10 days/second)
- **Softening Parameter**: 1×10³ (for numerical stability)
- **Integration Method**: Velocity Verlet with adaptive timestep

### Rendering Pipeline
- **Graphics API**: OpenGL 3.3 Core Profile
- **Shader Language**: GLSL 330
- **Geometry**: Subdivided icosahedron spheres
- **Lighting**: Flat shading with custom colors

### Performance Metrics
- **Target FPS**: 60 FPS
- **Planet Limit**: 10 active bodies (configurable)
- **Grid Resolution**: 200×200 vertices
- **Memory Usage**: ~50MB typical

## 🏗️ Architecture

The project follows a clean modular architecture:

```
SolarSystemGL/
├── src/
│   ├── core/           # Core engine components
│   ├── objects/        # Game objects (planets)
│   ├── physics/        # Physics simulation
│   ├── ui/             # User interface
│   └── main.cpp        # Application entry point
├── shaders/            # OpenGL shaders
├── third_party/        # External libraries
└── docs/               # Documentation
```

## 🌌 Celestial Bodies

The simulation includes all major planets with accurate data:

| Planet  | Mass (kg)     | Density (kg/m³) | Distance (AU) | Color        |
|---------|---------------|-----------------|---------------|--------------|
| Sun     | 1.989×10³⁰    | 1,408          | 0.000         | Yellow-Orange|
| Mercury | 3.301×10²³    | 5,427          | 0.387         | Gray         |
| Venus   | 4.868×10²⁴    | 5,243          | 0.723         | Pale Yellow  |
| Earth   | 5.972×10²⁴    | 5,514          | 1.000         | Blue         |
| Mars    | 6.417×10²³    | 3,933          | 1.524         | Red-Orange   |
| Jupiter | 1.898×10²⁷    | 1,326          | 5.203         | Orange-Brown |
| Saturn  | 5.683×10²⁶    | 687            | 9.537         | Pale Gold    |
| Uranus  | 8.681×10²⁵    | 1,271          | 19.191        | Cyan         |
| Neptune | 1.024×10²⁶    | 1,638          | 30.070        | Blue         |

## 🔬 Educational Value

This simulation serves as an excellent educational tool for:
- **Orbital Mechanics**: Understand how gravity shapes planetary orbits
- **Computer Graphics**: Learn OpenGL rendering techniques
- **Physics Simulation**: Explore numerical integration methods
- **Software Engineering**: Study modular C++ architecture

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details on:
- Code style guidelines
- Pull request process
- Bug reporting
- Feature requests

## 📚 Documentation

- **[User Guide](user-guide.md)**: Detailed usage instructions
- **[Technical Reference](technical-reference.md)**: In-depth technical documentation
- **[API Reference](api-reference.md)**: Complete class and method documentation
- **[Build Guide](build-guide.md)**: Comprehensive build instructions
- **[Physics Guide](physics.md)**: Physics simulation details

## 🐛 Known Issues

- Grid distortion may cause visual artifacts at extreme zoom levels
- Performance may degrade with more than 10 planets
- Some UI elements may not scale properly on high-DPI displays

## 🔮 Future Enhancements

- [ ] Asteroid belt simulation
- [ ] Planetary rings (Saturn)
- [ ] Comet trajectories
- [ ] Texture mapping for planets
- [ ] Multi-star systems

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **OpenGL**: For providing the graphics foundation
- **GLFW**: For window management and input handling
- **GLM**: For mathematical operations
- **ImGui**: For the immediate mode GUI
- **NASA**: For providing accurate planetary data

---

**Made with ❤️ for space enthusiasts and developers** 
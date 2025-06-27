# User Guide

Complete guide for using SolarSystemGL, covering installation, controls, features, and troubleshooting.

## 🚀 Getting Started

### System Requirements

**Minimum Requirements:**
- Windows 10 or later
- DirectX 11 compatible graphics card
- 2GB RAM
- 500MB free disk space
- Visual Studio 2019 or later (for building)

**Recommended Requirements:**
- Windows 11
- Dedicated graphics card with OpenGL 3.3+ support
- 4GB RAM
- Intel Core i5 or AMD Ryzen 5 processor
- Visual Studio 2022

### Installation

1. **Download the Project**
   ```bash
   git clone https://github.com/yourusername/SolarSystemGL.git
   cd SolarSystemGL
   ```

2. **Build the Application**
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release
   ```

3. **Run the Simulation**
   ```bash
   ./Release/SolarSystemGL.exe
   ```

## 🎮 Controls and Navigation

### Camera Controls

#### Keyboard Navigation
- **W**: Move camera forward
- **S**: Move camera backward
- **A**: Move camera left (strafe)
- **D**: Move camera right (strafe)
- **ESC**: Exit application

#### Mouse Controls
- **Left Click**: Select planet (when clicking on planet)
- **Right Click + Drag**: Rotate camera view
- **Scroll Wheel**: Zoom in/out
- **Hover**: Display planet name labels

### Camera Movement Tips
- Use **WASD** for basic movement around the solar system
- **Right-click and drag** to look around - this is essential for navigation
- **Scroll wheel** provides quick zoom for different viewing distances
- **Click on planets** to automatically focus the camera on them

## 🌟 Main Features

### 1. Planet Selection and Information

**How to Select Planets:**
1. Click directly on any planet in the 3D view
2. Use the "Planets" menu in the top menu bar
3. Selected planets are highlighted and the camera focuses on them

**Planet Information Panel:**
When a planet is selected, the "Planet Info" panel appears showing:
- **Name**: Editable planet name
- **Mass**: Mass in kilograms (scientific notation)
- **Density**: Density in kg/m³
- **Position**: X, Y, Z coordinates in world units
- **Velocity**: Velocity vector components

**Editing Planet Properties:**
1. Select a planet
2. Modify values in the Planet Info panel
3. Click **"Apply"** to confirm changes
4. Click **"Reset"** to revert to original values

### 2. Physics Simulation

**Real-Time Simulation:**
- All planets move according to Newtonian gravity
- Time is accelerated: 1 second real time = 10 days simulation time
- Orbital mechanics are accurately simulated

**Observable Phenomena:**
- Planetary orbits around the Sun
- Gravitational interactions between planets
- Orbital periods matching real astronomical data

### 3. Interactive Grid System

**Spacetime Visualization:**
- White grid represents space
- Grid deforms near massive objects (planets)
- Demonstrates gravitational field effects
- Grid distortion is proportional to planet mass

**Grid Properties:**
- Size: 10,000 world units
- Resolution: 200×200 grid lines
- Semi-transparent white appearance

### 4. User Interface Elements

#### Main Menu Bar
- **Planets**: Quick access to all planets
- **Camera**: Camera controls and settings
- **Settings**: Display options and preferences

#### Solar System Panel
- **FPS Counter**: Shows current frame rate
- **Add Planet**: Creates new custom planets
- Performance monitoring

#### Planet Labels
- Appear when hovering over planets
- Show planet names
- Automatically positioned above planets

## 🔧 Advanced Features

### Adding Custom Planets

1. Click **"Add Planet"** in the Solar System panel
2. A new planet appears with default properties:
   - Name: "New Planet"
   - Mass: 1×10²⁴ kg
   - Density: 3000 kg/m³
   - Position: Offset from existing planets

3. Select the new planet and edit its properties
4. Apply changes to see the effect on the simulation

### Camera Focus System

**Automatic Focus:**
- Click any planet to smoothly move camera to it
- Camera positions at optimal viewing distance
- Maintains planet in center of view

**Manual Focus:**
- Use keyboard controls for free movement
- Right-click drag for precise viewing angles
- Scroll wheel for distance adjustment

### Performance Optimization

**Frame Rate Management:**
- Target: 60 FPS
- Monitor FPS in Solar System panel
- Performance depends on planet count and graphics hardware

**Graphics Settings:**
- Grid can be toggled in Settings menu
- Planet detail level is fixed (subdivision level 3)

## 🐛 Troubleshooting

### Common Issues

#### Application Won't Start
**Symptoms:** Application crashes immediately or fails to launch

**Solutions:**
1. **Check Graphics Drivers:**
   - Update to latest graphics drivers
   - Ensure OpenGL 3.3+ support

2. **Verify Build:**
   - Rebuild in Release mode
   - Check for missing DLL files
   - Ensure shaders folder is copied to build directory

3. **System Compatibility:**
   - Confirm Windows 10+ compatibility
   - Check DirectX installation

#### Low Frame Rate
**Symptoms:** FPS below 30, choppy animation

**Solutions:**
1. **Graphics Settings:**
   - Update graphics drivers
   - Close other applications
   - Check GPU temperature

2. **System Performance:**
   - Ensure application has graphics hardware acceleration
   - Check CPU usage
   - Verify adequate RAM

3. **Application Settings:**
   - Reduce number of custom planets
   - Monitor system resources

#### Planets Not Moving
**Symptoms:** Planets appear static, no orbital motion

**Solutions:**
1. **Check Time Scale:**
   - Simulation uses accelerated time
   - Observable motion may take several seconds

2. **Physics Simulation:**
   - Verify physics system is active
   - Check for simulation errors in console

#### Controls Not Responding
**Symptoms:** Camera doesn't move, mouse input ignored

**Solutions:**
1. **Focus Issues:**
   - Click on 3D view area to ensure focus
   - Check if ImGui panels are capturing input

2. **Input System:**
   - Verify GLFW input callbacks
   - Check for conflicting applications

#### Visual Artifacts
**Symptoms:** Flickering, strange graphics, distorted rendering

**Solutions:**
1. **Graphics Issues:**
   - Update graphics drivers
   - Check OpenGL error messages
   - Verify shader compilation

2. **Hardware Problems:**
   - Check GPU temperature
   - Test with different graphics settings
   - Verify graphics card compatibility

### Performance Tuning

#### Optimal Settings
- **Planet Count:** 9-12 planets for best performance
- **Grid Resolution:** Default 200×200 is optimal
- **Camera Distance:** Stay within reasonable zoom limits

#### System Resources
- **RAM Usage:** ~50-100MB typical
- **CPU Usage:** ~10-20% on modern systems
- **GPU Usage:** Depends on graphics card

### Debug Information

#### Console Output
The application outputs debug information to console:
- OpenGL version and renderer
- Shader compilation status
- Frame timing information
- Error messages

#### Log Files
Check for log files in the application directory:
- Build logs from CMake
- Runtime error logs
- Performance profiling data

## 📊 Understanding the Simulation

### Scale and Units

**Distance Scale:**
- 1 World Unit = 1 billion meters (1 Gm)
- 1 Astronomical Unit (AU) ≈ 149.6 World Units
- Earth-Sun distance = 1 AU = 149.6 million km

**Time Scale:**
- 1 real second = 10 simulation days
- 1 real minute ≈ 200 simulation days
- Mercury orbit (88 days) completes in ~26 seconds

**Mass Scale:**
- All masses use real astronomical values
- Sun: 1.989×10³⁰ kg
- Earth: 5.972×10²⁴ kg

### Physics Accuracy

**What's Accurate:**
- Gravitational forces between all planets
- Orbital mechanics and periods
- Relative planet sizes and masses
- N-body gravitational interactions

**What's Simplified:**
- Circular initial orbits (real orbits are elliptical)
- No moons or asteroids
- No relativistic effects
- Simplified atmospheric effects

### Educational Value

**Learning Objectives:**
- Understand gravitational forces
- Observe orbital mechanics
- Explore scale of solar system
- Learn physics simulation concepts

**Experiments to Try:**
1. **Planet Removal:** Remove Jupiter and observe effect on other planets
2. **Mass Changes:** Increase Earth's mass and see orbital changes
3. **New Planets:** Add planets between existing orbits
4. **Binary Systems:** Create two-star systems

## 🎯 Best Practices

### Efficient Usage
- Start with default solar system
- Make small changes to observe effects
- Use planet selection for detailed examination
- Monitor performance with FPS counter

### Exploration Tips
- Begin with overview of entire system
- Focus on individual planets for detail
- Observe grid distortion near massive objects
- Experiment with custom planet properties

### Educational Applications
- Demonstrate gravitational concepts
- Show scale relationships in space
- Illustrate orbital mechanics
- Explore N-body dynamics

---

**This user guide provides comprehensive information for effectively using and understanding SolarSystemGL.** 
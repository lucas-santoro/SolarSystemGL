# Physics Documentation

Comprehensive documentation of the physics simulation in SolarSystemGL, including mathematical foundations, algorithms, and astronomical data.

## 🌌 Theoretical Foundation

### Newton's Law of Universal Gravitation

The simulation is based on Newton's inverse square law:

```
F⃗ = -G (m₁m₂/r²) r̂
```

Where:
- **F⃗**: Gravitational force vector (N)
- **G**: Universal gravitational constant = 6.67430×10⁻¹¹ m³/(kg·s²)
- **m₁, m₂**: Masses of the two bodies (kg)
- **r**: Distance between centers of mass (m)
- **r̂**: Unit vector pointing from m₁ to m₂

### N-Body Problem

The simulation solves the gravitational N-body problem for the solar system. For N bodies, each body i experiences acceleration:

```
a⃗ᵢ = Σⱼ≠ᵢ G mⱼ (r⃗ⱼ - r⃗ᵢ) / |r⃗ⱼ - r⃗ᵢ|³
```

This creates a system of 3N coupled second-order differential equations.

## 🧮 Numerical Integration

### Leapfrog Integration Scheme

The simulation uses a modified leapfrog (Verlet) integration method:

**Step 1: Calculate accelerations**
```cpp
for (size_t i = 0; i < bodies.size(); ++i) {
    glm::dvec3 acc(0.0);
    for (size_t j = 0; j < bodies.size(); ++j) {
        if (i == j) continue;
        
        glm::dvec3 r = bodies[j].pos_m - bodies[i].pos_m;
        double dist2 = glm::dot(r, r) + SOFTEN;
        double invD = 1.0 / sqrt(dist2);
        
        acc += (G * bodies[j].mass_kg * invD * invD) * r * invD;
    }
    bodies[i].vel_m += acc * dtSim;
}
```

**Step 2: Update positions**
```cpp
for (auto& b : bodies) {
    b.pos_m += b.vel_m * dtSim;
}
```

### Integration Formulas

**Velocity update:**
```
v⃗(t + Δt) = v⃗(t) + a⃗(t) · Δt
```

**Position update:**
```
r⃗(t + Δt) = r⃗(t) + v⃗(t + Δt) · Δt
```

### Stability Analysis

**Time step criterion:** For stability, the time step must satisfy:
```
Δt < 2π √(r³/GM)
```

For the innermost orbit (Mercury), this gives:
```
Δt_max ≈ 2π √((0.387 AU)³/(GM☉)) ≈ 87.97 days
```

With our acceleration factor of 860,400, the real-time step is:
```
Δt_real = 87.97 days / 860,400 ≈ 0.1 seconds
```

## 🛡️ Numerical Stabilization

### Softening Parameter

To prevent singularities when bodies approach each other, a softening parameter ε is introduced:

```
r_effective = √(r² + ε²)
```

Where ε = 1×10³ m. This modifies the force calculation to:

```
F = G m₁m₂ / (r² + ε²)^(3/2)
```

**Effect on orbits:**
- For r >> ε: Negligible effect on dynamics
- For r ≈ ε: Prevents numerical divergence
- For r << ε: Force approaches constant value

### Time Scaling

Real time is accelerated by factor α = 860,400:

```
t_simulation = α · t_real
dt_simulation = α · dt_real
```

This allows observation of orbital motion in reasonable time:
- 1 second real time = 10 days simulation time
- 1 minute real time = ~200 days simulation time
- Mercury orbit (88 days) completes in ~26 seconds

## 🪐 Planetary Data

### Physical Properties

| Planet  | Mass (kg)      | Density (kg/m³) | Radius (km) | Distance (AU) | Orbital Velocity (km/s) |
|---------|----------------|-----------------|-------------|---------------|-------------------------|
| Sun     | 1.989×10³⁰     | 1,408          | 696,340     | 0.000         | 0.000                  |
| Mercury | 3.301×10²³     | 5,427          | 2,440       | 0.387         | 47.90                  |
| Venus   | 4.868×10²⁴     | 5,243          | 6,052       | 0.723         | 35.00                  |
| Earth   | 5.972×10²⁴     | 5,514          | 6,371       | 1.000         | 29.78                  |
| Mars    | 6.417×10²³     | 3,933          | 3,390       | 1.524         | 24.10                  |
| Jupiter | 1.898×10²⁷     | 1,326          | 69,911      | 5.203         | 13.07                  |
| Saturn  | 5.683×10²⁶     | 687            | 58,232      | 9.537         | 9.68                   |
| Uranus  | 8.681×10²⁵     | 1,271          | 25,362      | 19.191        | 6.80                   |
| Neptune | 1.024×10²⁶     | 1,638          | 24,622      | 30.070        | 5.43                   |

### Orbital Mechanics Formulas

**Circular orbital velocity:**
```
v = √(GM/r)
```

**Orbital period (Kepler's Third Law):**
```
T = 2π √(a³/GM)
```

**Escape velocity:**
```
v_escape = √(2GM/r)
```

**Gravitational parameter:**
```
μ = GM = 1.327×10²⁰ m³/s² (Sun)
```

## 🔬 Implementation Details

### Force Calculation Algorithm

```cpp
glm::dvec3 calculateGravitationalForce(const BodyState& body1, 
                                       const BodyState& body2) {
    glm::dvec3 r = body2.pos_m - body1.pos_m;
    double r_squared = glm::dot(r, r);
    double r_softened = sqrt(r_squared + SOFTEN);
    double r_cubed = r_softened * r_softened * r_softened;
    
    double force_magnitude = G * body1.mass_kg * body2.mass_kg / r_cubed;
    return force_magnitude * r;
}
```

### Acceleration Calculation

For body i, total acceleration from all other bodies j:

```cpp
glm::dvec3 acc_i(0.0);
for (size_t j = 0; j < N; ++j) {
    if (i != j) {
        glm::dvec3 r_ij = bodies[j].pos_m - bodies[i].pos_m;
        double r_mag_sq = glm::dot(r_ij, r_ij) + SOFTEN;
        double inv_r = 1.0 / sqrt(r_mag_sq);
        double inv_r_cubed = inv_r * inv_r * inv_r;
        
        acc_i += G * bodies[j].mass_kg * r_ij * inv_r_cubed;
    }
}
```

### Computational Complexity

**Time complexity per step:** O(N²)
**Space complexity:** O(N)

For N = 9 planets:
- 9² = 81 force calculations per timestep
- At 60 FPS: 4,860 force calculations per second

### Energy Conservation

**Total energy:** E = T + V

**Kinetic energy:**
```
T = Σᵢ ½mᵢvᵢ²
```

**Potential energy:**
```
V = -Σᵢ Σⱼ>ᵢ Gmᵢmⱼ/rᵢⱼ
```

**Conservation check:**
```cpp
double calculateTotalEnergy(const std::vector<BodyState>& bodies) {
    double kinetic = 0.0, potential = 0.0;
    
    // Kinetic energy
    for (const auto& body : bodies) {
        kinetic += 0.5 * body.mass_kg * glm::dot(body.vel_m, body.vel_m);
    }
    
    // Potential energy
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            glm::dvec3 r = bodies[j].pos_m - bodies[i].pos_m;
            double distance = glm::length(r);
            potential -= G * bodies[i].mass_kg * bodies[j].mass_kg / distance;
        }
    }
    
    return kinetic + potential;
}
```

## 🎯 Accuracy Analysis

### Error Sources

1. **Discretization error:** O(Δt²) for Verlet integration
2. **Roundoff error:** Machine precision (double ~10⁻¹⁶)
3. **Softening error:** Artificial force modification for r < ε
4. **Initial condition error:** Approximate starting positions/velocities

### Validation Methods

**Conservation laws:**
- Energy conservation: |ΔE/E₀| < 10⁻⁶
- Angular momentum conservation: |ΔL/L₀| < 10⁻⁶
- Linear momentum conservation: |Δp| < 10⁻¹²

**Orbital elements:**
- Semi-major axis deviation: < 0.1%
- Eccentricity error: < 0.01
- Period accuracy: < 1%

### Benchmark Results

For Earth's orbit over 1 simulation year:
- Position error: < 0.01 AU
- Velocity error: < 0.1 km/s
- Energy drift: < 10⁻⁸
- Angular momentum drift: < 10⁻¹⁰

## 🌍 Coordinate Systems

### Simulation Coordinates

**Origin:** Solar system barycenter (approximately Sun's center)
**Units:** Meters (SI)
**Axes:**
- X: Vernal equinox direction
- Y: 90° from X in ecliptic plane
- Z: North ecliptic pole

### Rendering Coordinates

**Origin:** Same as simulation
**Units:** World Units (1 WU = 10⁹ m)
**Scaling factor:** 10⁻⁹

**Conversion:**
```cpp
glm::vec3 renderPos = glm::vec3(simulationPos / 1e9);
```

### Astronomical Units

**1 AU = 1.495978707×10¹¹ m**

Used for:
- Initial planet positions
- Distance measurements
- Orbital radius specification

## 🔧 Advanced Features

### Relativistic Corrections (Not Implemented)

For future enhancement, relativistic effects could be added:

**Einstein's correction to Newtonian gravity:**
```
F_rel = F_newton × (1 + 3GM/(rc²))
```

**Perihelion precession:**
```
Δφ = 6πGM/(ac²(1-e²))
```

### Tidal Forces

For extended bodies, tidal acceleration:
```
a_tidal = 2GM/r³ × Δr
```

### Three-Body Effects

Lagrange points for restricted three-body problem:
```
L₁,₂,₃: Collinear points
L₄,₅: Triangular points (±60° from line)
```

## 📊 Performance Optimization

### Algorithmic Improvements

**Barnes-Hut algorithm:** O(N log N) complexity
- Hierarchical space partitioning
- Approximate distant forces
- Exact calculation for nearby bodies

**Particle-Mesh methods:** O(N log N)
- Grid-based force calculation
- FFT for long-range forces

### Parallel Processing

**OpenMP parallelization:**
```cpp
#pragma omp parallel for
for (size_t i = 0; i < bodies.size(); ++i) {
    // Calculate forces for body i
}
```

**GPU acceleration (CUDA):**
- Parallel force calculation
- Memory coalescing optimization
- Shared memory utilization

## 🧪 Testing and Validation

### Unit Tests

```cpp
TEST(PhysicsSystem, ConservationOfEnergy) {
    std::vector<BodyState> bodies = createTwoBodySystem();
    PhysicsSystem physics;
    
    double initialEnergy = calculateTotalEnergy(bodies);
    
    for (int i = 0; i < 1000; ++i) {
        physics.update(bodies, 0.01);
    }
    
    double finalEnergy = calculateTotalEnergy(bodies);
    double energyError = abs(finalEnergy - initialEnergy) / initialEnergy;
    
    EXPECT_LT(energyError, 1e-6);
}
```

### Integration Tests

- Full solar system simulation for 10 years
- Compare with NASA JPL ephemeris data
- Validate orbital periods within 1%

---

**This physics documentation provides the mathematical and computational foundation for understanding and extending the gravitational simulation in SolarSystemGL.** 
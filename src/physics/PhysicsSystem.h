#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "objects/CelestialBody.h"

/**
 * @file PhysicsSystem.h
 * @brief Newtonian N-body integrator with Velocity Verlet and fixed timestep.
 */

/**
 * @brief Drives the N-body simulation.
 *
 * Each call to #update consumes real-time `dtReal`, accumulates it into an
 * internal buffer, and consumes the buffer in fixed `FIXED_DT_REAL` chunks
 * (Glenn Fiedler "Fix Your Timestep"). Each chunk runs one full kick-drift-kick
 * Velocity Verlet step using `timeScale * FIXED_DT_REAL` as the simulated
 * duration.
 *
 * The accumulator is capped at #MAX_ACCUM to avoid the spiral of death after
 * long suspensions (debugger, OS hibernation).
 *
 * `timeScale` and `paused` are public so ImGui can bind them directly to a
 * slider / checkbox.
 */
class PhysicsSystem
{
public:
    /** @brief Simulated seconds per real second. Bound to the UI slider. */
    float timeScale = 864'000.0f;

    /** @brief When true, #update returns immediately without integrating. */
    bool  paused = false;

    /** @brief Newton's gravitational constant in SI units (m^3 / (kg * s^2)). */
    static constexpr double G = 6.67430e-11;

    /** @brief Softening term in m^2 added to dist^2 to avoid the 1/r singularity. */
    static constexpr double SOFTEN = 1e3;

    /** @brief Real-time duration of one physics sub-step (~4.16 ms). */
    static constexpr double FIXED_DT_REAL = 1.0 / 240.0;

    /** @brief Maximum accumulated real time before sub-step catch-up is capped. */
    static constexpr double MAX_ACCUM = 0.5;

    /**
     * @brief Advance the simulation by @p dtReal real-time seconds.
     *
     * No-op when #paused. Otherwise accumulates `dtReal` and runs as many
     * fixed sub-steps as fit, each integrating the bodies via Velocity Verlet.
     *
     * @param bodies Vector of bodies — both read (positions, masses) and mutated (positions, velocities).
     * @param dtReal Real-time delta for this frame, in seconds.
     */
    void update(std::vector<CelestialBody>& bodies, double dtReal);

    /// @return Total simulated seconds advanced since the last #reset call.
    double getSimulatedTimeSeconds() const { return simulatedTime_; }

    /// Reset the accumulator and the simulated-time counter (Application calls
    /// this when starting a new simulation or returning to the menu).
    void reset();

private:
    double accumulator    = 0.0;
    double simulatedTime_ = 0.0;
    std::vector<glm::dvec3> accBuffer0;
    std::vector<glm::dvec3> accBuffer1;

    void stepOnce(std::vector<CelestialBody>& bodies, double dtReal);
    void computeAccelerations(const std::vector<CelestialBody>& bodies,
                              std::vector<glm::dvec3>& out) const;
};

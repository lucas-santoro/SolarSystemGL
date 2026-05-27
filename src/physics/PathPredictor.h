#pragma once

#include <glm/glm.hpp>

#include <vector>

struct CelestialBody;

/**
 * @file PathPredictor.h
 * @brief Shadow N-body integrator that previews a body's future trajectory.
 */

/**
 * @brief Integrates a copy of the simulation forward in time without touching
 *        the real bodies and stores the selected body's positions for line-strip
 *        rendering.
 *
 * Same Velocity Verlet integrator as `PhysicsSystem`, but runs on a shadow
 * vector so the simulation is unaffected. Cost scales as `O(sampleCount × N²)`
 * with the number of bodies, which is why the host throttles recomputation
 * (typically once every ~30 frames).
 */
class PathPredictor
{
public:
    /**
     * @brief Replace the cached path with a fresh prediction.
     *
     * @param bodies          Live bodies whose state is snapshotted as the starting point.
     * @param selectedIdx     Index of the body whose path should be sampled.
     * @param horizonSeconds  How far into the simulated future to integrate.
     * @param sampleCount     Number of points along the predicted path.
     */
    void recompute(const std::vector<CelestialBody>& bodies, int selectedIdx, double horizonSeconds,
                   int sampleCount);

    /**
     * @brief Estimate the Keplerian orbital period of @p selectedIdx around
     *        the most massive other body.
     *
     * `T = 2π √(r³ / (G·M))` where M is the dominant mass and r is the
     * current separation. Returns 0.0 if no central mass is found.
     */
    static double estimateOrbitalPeriod(const std::vector<CelestialBody>& bodies, int selectedIdx);

    /// Drop the cached path. Call when prediction is disabled or invalid.
    void clear()
    {
        path_.clear();
    }

    /// @return The predicted body positions in world units (line-strip order).
    const std::vector<glm::vec3>& getPath() const
    {
        return path_;
    }

private:
    struct ShadowState
    {
        glm::dvec3 pos_m;
        glm::dvec3 vel_m;
        double mass_kg;
    };

    std::vector<ShadowState> shadowBodies_;
    std::vector<glm::dvec3> accBuffer0_;
    std::vector<glm::dvec3> accBuffer1_;
    std::vector<glm::vec3> path_;

    void computeAccelerations(std::vector<glm::dvec3>& out) const;
    void stepOnce(double dtSim);
};

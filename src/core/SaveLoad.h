#pragma once
#include <string>
#include <vector>
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"

// Custom INI-like format. One global section at the top (version, timeScale),
// then one [body] section per CelestialBody. Whitespace-tolerant, '#' comments,
// keys are case-sensitive. See docs/api/SaveLoad.md for the schema.

bool saveSimulation(const std::string &path,
                    const std::vector<CelestialBody> &bodies,
                    const PhysicsSystem &physics);

// On success, `bodies` and `physics.timeScale` are overwritten. On failure
// (file not openable, no [body] sections found, etc.), nothing is modified.
bool loadSimulation(const std::string &path,
                    std::vector<CelestialBody> &bodies,
                    PhysicsSystem &physics);

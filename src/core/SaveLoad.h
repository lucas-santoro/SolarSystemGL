#pragma once

#include <string>
#include <vector>
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"

/**
 * @file SaveLoad.h
 * @brief Persistence layer for the simulation state.
 *
 * Custom INI-like text format. One global section at the top (version,
 * timeScale), then one `[body]` section per CelestialBody. Whitespace-tolerant,
 * `#` comments allowed, keys are case-sensitive. See `docs/api/SaveLoad.md`
 * for the full schema.
 */

/**
 * @brief Write @p bodies and @p physics.timeScale to a text file.
 *
 * Uses `std::scientific` with 15 digits of precision so doubles round-trip
 * without loss.
 *
 * @param path    Filesystem path of the file to create (or overwrite).
 * @param bodies  Bodies to serialize, in order.
 * @param physics Physics system — only `timeScale` is written.
 * @return        `true` on success; `false` if the file cannot be opened for writing.
 */
bool saveSimulation(const std::string& path,
                    const std::vector<CelestialBody>& bodies,
                    const PhysicsSystem& physics);

/**
 * @brief Replace @p bodies and @p physics.timeScale from a text file.
 *
 * Atomic on failure: if the file cannot be opened or contains zero `[body]`
 * sections, neither argument is modified. On success, every loaded body's
 * mesh is regenerated via `CelestialBody::recalculateGeometry` before the swap.
 *
 * @param path    Filesystem path of the file to read.
 * @param bodies  Output container — replaced on success.
 * @param physics Physics system — only `timeScale` is overwritten.
 * @return        `true` on success; `false` if the file cannot be opened or contains no bodies.
 */
bool loadSimulation(const std::string& path,
                    std::vector<CelestialBody>& bodies,
                    PhysicsSystem& physics);

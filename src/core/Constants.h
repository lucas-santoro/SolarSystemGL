#pragma once

/**
 * @file Constants.h
 * @brief Unit-conversion constants used across physics, render, and UI layers.
 *
 * The simulation stores positions in **meters** (double precision) but the GL
 * pipeline runs in **world units** (single precision) where one world unit
 * equals 10^9 meters. This file defines those scales and the corresponding
 * astronomical unit shorthand.
 */

/** @brief Meters per world unit. Render coordinates = pos_m / METERS_PER_WU. */
inline constexpr double METERS_PER_WU = 1.0e9;

/** @brief One astronomical unit in meters (IAU 2012 definition). */
inline constexpr double AU = 1.495978707e11;

/** @brief One astronomical unit expressed in world units (~149.6). */
inline constexpr float AU_WU = static_cast<float>(AU / METERS_PER_WU);

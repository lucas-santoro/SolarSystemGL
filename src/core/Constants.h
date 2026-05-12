#pragma once

inline constexpr double METERS_PER_WU = 1.0e9;
inline constexpr double AU            = 1.495978707e11;
inline constexpr float  AU_WU         = static_cast<float>(AU / METERS_PER_WU);

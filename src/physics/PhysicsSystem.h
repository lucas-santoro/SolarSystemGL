#pragma once
#include <vector>
#include "BodyState.h"

class PhysicsSystem
{
public:
    double timeScale = 860'400.0; // 10 days / s 

    static constexpr double G = 6.67430e-11;
    static constexpr double SOFTEN = 1e3;

    void update(std::vector<BodyState>& bodies, double dtReal) const;
};

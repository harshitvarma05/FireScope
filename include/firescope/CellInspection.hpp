#pragma once
#include "SimulationTypes.hpp"
#include <algorithm>
#include <cmath>
namespace firescope {
struct IgnitionProbe { unsigned burningNeighbors = 0; float probability = 0; };
// Diagnostic evaluation of the same simplified neighbor model used by all backends.
// This reads a captured grid; it never changes simulation state or the random stream.
inline IgnitionProbe inspectIgnition(const GridData& grid, const SimulationConfig& config,
                                    const EnvironmentParams& p, uint32_t x, uint32_t y) {
    IgnitionProbe result;
    const size_t i = static_cast<size_t>(y)*config.width+x;
    float survival = 1;
    const float moisture = 1-(1-grid.moisture[i])*(1-p.globalHumidity);
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
        if (!dx && !dy) continue;
        const int nx = static_cast<int>(x)+dx, ny = static_cast<int>(y)+dy;
        if (nx < 0 || ny < 0 || nx >= static_cast<int>(config.width) || ny >= static_cast<int>(config.height)) continue;
        const size_t j = static_cast<size_t>(ny)*config.width+nx;
        if (grid.state[j] != static_cast<uint32_t>(CellState::Burning)) continue;
        ++result.burningNeighbors;
        const float angle = std::atan2(static_cast<float>(-dy),static_cast<float>(-dx));
        const float distance = p.cellSizeMeters*((dx && dy) ? 1.41421356237f : 1.0f);
        const float slope = std::atan((grid.elevation[i]-grid.elevation[j])/distance);
        const float exponent = p.windStrength*p.windSpeed+p.windSpeed*p.windAlignmentStrength*
            (std::cos(angle-p.windDirectionRadians)-1)+p.slopeStrength*slope-p.moistureStrength*moisture;
        const float probability = std::clamp(p.baseIgnitionProbability*(0.5f+grid.vegetation[i])*
            (0.5f+grid.density[i])*std::exp(std::clamp(exponent,-80.0f,80.0f)),0.0f,1.0f);
        survival *= 1-probability;
    }
    if (grid.state[i] == static_cast<uint32_t>(CellState::Unburned) && grid.fuel[i] > 0)
        result.probability = 1-survival;
    return result;
}
}

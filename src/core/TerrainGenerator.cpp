#include "firescope/Scenario.hpp"
#include "firescope/DeterministicRNG.hpp"
#include <cmath>
namespace firescope {
InitialGrid generateScenario(const SimulationConfig& config, uint32_t seed) {
    InitialGrid grid;
    const size_t n = static_cast<size_t>(config.width) * config.height;
    grid.state.assign(n, static_cast<uint32_t>(CellState::Unburned));
    grid.burnTicks.assign(n, 0);
    grid.fuel.resize(n); grid.moisture.resize(n); grid.elevation.resize(n);
    grid.vegetation.resize(n); grid.density.resize(n);
    for (uint32_t y = 0; y < config.height; ++y) {
        for (uint32_t x = 0; x < config.width; ++x) {
            const size_t i = static_cast<size_t>(y) * config.width + x;
            const float r = (hashCell(seed, x, y, 0) >> 8) / 16777216.0f;
            grid.fuel[i] = 0.7f + 0.3f * r;
            grid.moisture[i] = 0.05f + 0.15f * r;
            grid.vegetation[i] = 0.6f + 0.4f * r;
            grid.density[i] = 0.4f + 0.6f * r;
            grid.elevation[i] = 30.0f * std::sin(x * 0.065f) * std::cos(y * 0.045f)
                              + 10.0f * std::sin((x + y) * 0.13f);
        }
    }
    return grid;
}
}

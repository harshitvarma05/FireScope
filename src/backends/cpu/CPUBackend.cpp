#include "CPUBackend.hpp"
#include "firescope/Validation.hpp"
#include "firescope/DeterministicRNG.hpp"
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace firescope {
namespace {
constexpr uint32_t state(CellState s) { return static_cast<uint32_t>(s); }
void validate(const GridData& g, size_t n) {
    if (g.state.size() != n || g.fuel.size() != n || g.moisture.size() != n ||
        g.elevation.size() != n || g.vegetation.size() != n ||
        g.density.size() != n || g.burnTicks.size() != n)
        throw std::invalid_argument("Grid field sizes do not match dimensions");
    for (size_t i = 0; i < n; ++i) {
        if (g.state[i] > state(CellState::Burned) || !std::isfinite(g.fuel[i]) || g.fuel[i] < 0 ||
            !std::isfinite(g.elevation[i]) || !std::isfinite(g.moisture[i]) ||
            g.moisture[i] < 0 || g.moisture[i] > 1 || !std::isfinite(g.vegetation[i]) ||
            g.vegetation[i] < 0 || g.vegetation[i] > 1 || !std::isfinite(g.density[i]) ||
            g.density[i] < 0 || g.density[i] > 1)
            throw std::invalid_argument("Invalid cell property");
    }
}
}
void CPUBackend::initialize(const SimulationConfig& config, const InitialGrid& grid) {
    if (!config.width || !config.height || config.width > 2048 || config.height > 2048)
        throw std::invalid_argument("Grid dimensions must be between 1 and 2048");
    validate(grid, static_cast<size_t>(config.width) * config.height);
    config_ = config; initialized_ = true; reset(grid);
}
void CPUBackend::reset(const InitialGrid& grid) {
    if (!initialized_) throw std::logic_error("Initialize backend before reset");
    validate(grid, static_cast<size_t>(config_.width) * config_.height);
    current_ = grid; next_ = grid; stats_ = {}; countStates();
}
void CPUBackend::countStates() {
    stats_.noFuel = stats_.unburned = stats_.burning = stats_.burned = 0;
    for (auto s : current_.state) {
        switch (static_cast<CellState>(s)) {
        case CellState::NoFuel: ++stats_.noFuel; break;
        case CellState::Unburned: ++stats_.unburned; break;
        case CellState::Burning: ++stats_.burning; break;
        case CellState::Burned: ++stats_.burned; break;
        }
    }
}
void CPUBackend::step(const EnvironmentParams& p, uint32_t stepCount) {
    if (!initialized_) throw std::logic_error("Initialize backend before step");
    validateEnvironment(p);
    if (!stepCount) return;
    auto begin = std::chrono::steady_clock::now();
    for (uint32_t step = 0; step < stepCount; ++step) {
        for (uint32_t y = 0; y < config_.height; ++y) {
            for (uint32_t x = 0; x < config_.width; ++x) {
                const size_t i = static_cast<size_t>(y) * config_.width + x;
                const uint32_t s = current_.state[i];
                next_.state[i] = s; next_.fuel[i] = current_.fuel[i];
                next_.burnTicks[i] = current_.burnTicks[i];
                if (s == state(CellState::Burning)) {
                    next_.fuel[i] = std::max(0.0f, current_.fuel[i] - p.fuelConsumptionRate);
                    next_.burnTicks[i] = current_.burnTicks[i] + 1;
                    if (next_.fuel[i] <= 0 && next_.burnTicks[i] >= p.minimumBurnTicks)
                        next_.state[i] = state(CellState::Burned);
                } else if (s == state(CellState::Unburned) && current_.fuel[i] > 0) {
                    float survival = 1.0f;
                    const float moisture = 1.0f - (1.0f - current_.moisture[i]) * (1.0f - p.globalHumidity);
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (!dx && !dy) continue;
                            const int nx = static_cast<int>(x) + dx, ny = static_cast<int>(y) + dy;
                            if (nx < 0 || ny < 0 || nx >= static_cast<int>(config_.width) ||
                                ny >= static_cast<int>(config_.height)) continue;
                            const size_t j = static_cast<size_t>(ny) * config_.width + nx;
                            if (current_.state[j] != state(CellState::Burning)) continue;
                            // Direction FROM burning source TO target. +x = east, +y = north.
                            const float angle = std::atan2(static_cast<float>(-dy), static_cast<float>(-dx));
                            const float distance = p.cellSizeMeters * ((dx && dy) ? 1.41421356237f : 1.0f);
                            const float slope = std::atan((current_.elevation[i] - current_.elevation[j]) / distance);
                            const float exponent = p.windStrength * p.windSpeed +
                                p.windSpeed * p.windAlignmentStrength * (std::cos(angle - p.windDirectionRadians) - 1.0f) +
                                p.slopeStrength * slope - p.moistureStrength * moisture;
                            const float probability = std::clamp(p.baseIgnitionProbability *
                                (0.5f + current_.vegetation[i]) * (0.5f + current_.density[i]) *
                                std::exp(std::clamp(exponent, -80.0f, 80.0f)), 0.0f, 1.0f);
                            survival *= 1.0f - probability;
                        }
                    }
                    if (ignitionSucceeds(hashCell(p.seed, x, y, static_cast<uint32_t>(stats_.timestep)), 1.0f - survival)) {
                        next_.state[i] = state(CellState::Burning); next_.burnTicks[i] = 0;
                    }
                }
            }
        }
        // Only evolving fields swap; environment fields live in current_.
        current_.state.swap(next_.state); current_.fuel.swap(next_.fuel);
        current_.burnTicks.swap(next_.burnTicks); ++stats_.timestep;
    }
    countStates();
    stats_.simulationMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count() / stepCount;
}
void CPUBackend::downloadRenderState(RenderState& out) {
    out.width = config_.width; out.height = config_.height;
    out.cellState.resize(current_.state.size()); out.fireIntensity.resize(current_.state.size());
    for (size_t i = 0; i < current_.state.size(); ++i) {
        out.cellState[i] = static_cast<uint8_t>(current_.state[i]);
        out.fireIntensity[i] = current_.state[i] == state(CellState::Burning) ?
            static_cast<uint8_t>(std::clamp(current_.fuel[i], 0.15f, 1.0f) * 255.0f) : 0;
    }
}
void CPUBackend::applyBrush(BrushTool tool, int x, int y, int radius, float strength) {
    if (!initialized_) throw std::logic_error("Initialize backend before brush");
    if (radius < 0 || radius > 30 || !std::isfinite(strength) || strength < 0)
        throw std::invalid_argument("Invalid brush parameters");
    for (int cy = std::max(0, y - radius); cy <= std::min(static_cast<int>(config_.height) - 1, y + radius); ++cy) {
        for (int cx = std::max(0, x - radius); cx <= std::min(static_cast<int>(config_.width) - 1, x + radius); ++cx) {
            if ((cx - x) * (cx - x) + (cy - y) * (cy - y) > radius * radius) continue;
            const size_t i = static_cast<size_t>(cy) * config_.width + cx;
            if (tool == BrushTool::Ignite && current_.state[i] == state(CellState::Unburned) && current_.fuel[i] > 0) {
                current_.state[i] = state(CellState::Burning); current_.burnTicks[i] = 0;
            } else if (tool == BrushTool::Firebreak) {
                current_.state[i] = state(CellState::NoFuel); current_.fuel[i] = 0; current_.burnTicks[i] = 0;
            } else if (tool == BrushTool::Water) {
                current_.moisture[i] = std::min(1.0f, current_.moisture[i] + strength);
            }
        }
    }
    countStates();
}
}

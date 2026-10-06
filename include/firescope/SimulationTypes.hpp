#pragma once
#include <cstdint>
#include <vector>

namespace firescope {
enum class CellState : uint32_t { NoFuel = 0, Unburned = 1, Burning = 2, Burned = 3 };
struct SimulationConfig { uint32_t width = 128, height = 128; };
struct GridData {
    std::vector<uint32_t> state;
    std::vector<float> fuel, moisture, elevation, vegetation, density;
    std::vector<uint32_t> burnTicks;
};
using InitialGrid = GridData;
struct EnvironmentParams {
    float baseIgnitionProbability = 0.16f;
    float windSpeed = 3.0f;
    float windDirectionRadians = 0.0f;
    float globalHumidity = 0.1f;
    float moistureStrength = 3.0f;
    float slopeStrength = 2.0f;
    float windStrength = 0.06f;
    float windAlignmentStrength = 0.12f;
    float fuelConsumptionRate = 0.08f;
    float cellSizeMeters = 10.0f;
    uint32_t minimumBurnTicks = 3;
    uint32_t seed = 42;
    uint32_t timestep = 0;
};
struct RenderState {
    uint32_t width = 0, height = 0;
    std::vector<uint8_t> cellState, fireIntensity;
};
struct SimulationStats {
    uint64_t noFuel = 0, unburned = 0, burning = 0, burned = 0, timestep = 0;
    double simulationMs = 0.0, renderMs = 0.0, frameMs = 0.0;
};
enum class BrushTool { Ignite, Firebreak, Water, Vegetation };
}

#pragma once
#include "SimulationTypes.hpp"
#include "SimulationBackend.hpp"
namespace firescope {
InitialGrid generateScenario(const SimulationConfig&, uint32_t seed);
struct ScenarioSnapshot {
    SimulationConfig config;
    EnvironmentParams params;
    GridData grid;
    uint64_t timestep = 0;
};
struct ComparisonResult {
    SimulationStats baseline, mitigated;
    GridData mitigatedFinal;
};
ScenarioSnapshot captureScenario(ISimulationBackend&, const SimulationConfig&, const EnvironmentParams&);
ComparisonResult compareMitigation(const ScenarioSnapshot&, const GridData& mitigatedInitial,
                                   const std::string& backendName, uint32_t steps);
}

#include "firescope/Scenario.hpp"
#include "firescope/BackendFactory.hpp"
#include <stdexcept>
namespace firescope {
ScenarioSnapshot captureScenario(ISimulationBackend& backend,const SimulationConfig& config,const EnvironmentParams& params) {
    ScenarioSnapshot snapshot;
    snapshot.config=config; snapshot.params=params; snapshot.timestep=backend.stats().timestep;
    backend.downloadGrid(snapshot.grid); return snapshot;
}
ComparisonResult compareMitigation(const ScenarioSnapshot& snapshot,const GridData& mitigatedInitial,
    const std::string& backendName,uint32_t steps) {
    if (!steps) throw std::invalid_argument("Comparison must run at least one step");
    auto baseline=createBackend(backendName),mitigated=createBackend(backendName);
    baseline->initialize(snapshot.config,snapshot.grid); baseline->restoreSnapshot(snapshot.grid,snapshot.timestep);
    mitigated->initialize(snapshot.config,mitigatedInitial); mitigated->restoreSnapshot(mitigatedInitial,snapshot.timestep);
    baseline->step(snapshot.params,steps); mitigated->step(snapshot.params,steps);
    ComparisonResult result;
    result.baseline=baseline->stats(); result.mitigated=mitigated->stats();
    mitigated->downloadGrid(result.mitigatedFinal); return result;
}
}

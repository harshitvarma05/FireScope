#include "firescope/Scenario.hpp"
#include "firescope/BackendFactory.hpp"
#include <iostream>
#include <stdexcept>
using namespace firescope;
int main() {
    try {
        for (const auto& name : availableBackends()) {
            SimulationConfig config{31,21}; auto grid=generateScenario(config,42);
            grid.elevation.assign(grid.state.size(),0); grid.moisture.assign(grid.state.size(),0);
            grid.vegetation.assign(grid.state.size(),0.5f); grid.density.assign(grid.state.size(),0.5f);
            EnvironmentParams p; p.windSpeed=0; p.globalHumidity=0; p.slopeStrength=0;
            p.moistureStrength=0; p.baseIgnitionProbability=1;
            auto backend=createBackend(name); backend->initialize(config,grid);
            backend->applyBrush(BrushTool::Ignite,3,10,0); backend->step(p,3);
            auto snapshot=captureScenario(*backend,config,p);
            backend->step(p,4); RenderState expected; backend->downloadRenderState(expected);
            backend->restoreSnapshot(snapshot.grid,snapshot.timestep); backend->step(p,4);
            RenderState restored; backend->downloadRenderState(restored);
            if (expected.cellState!=restored.cellState || expected.fireIntensity!=restored.fireIntensity || backend->stats().timestep!=7)
                throw std::runtime_error("Snapshot did not preserve state, fuel, burn age and logical timestep");
            backend->restoreSnapshot(snapshot.grid,snapshot.timestep);
            for (int y=0;y<21;++y) backend->applyBrush(BrushTool::Firebreak,15,y,0);
            backend->applyBrush(BrushTool::Water,18,10,4,0.9f);
            GridData edited; backend->downloadGrid(edited);
            if (edited.moisture[10*31+18]<0.89f) throw std::runtime_error("Snapshot lost GPU moisture edits");
            auto result=compareMitigation(snapshot,edited,name,80);
            if (result.baseline.burned!=651 || result.mitigated.burned!=315 || result.mitigated.noFuel!=21 ||
                result.mitigated.unburned!=315 || result.baseline.timestep!=83 || result.mitigated.timestep!=83)
                throw std::runtime_error("Paired firebreak comparison returned incorrect counts/timesteps");
            backend->restoreSnapshot(snapshot.grid,snapshot.timestep);
            auto unchanged=compareMitigation(snapshot,snapshot.grid,name,20);
            if (unchanged.baseline.burned!=unchanged.mitigated.burned || unchanged.baseline.burning!=unchanged.mitigated.burning)
                throw std::runtime_error("Identical paired scenarios differ");
            if (snapshot.grid.state[10*31+15]!=1) throw std::runtime_error("Comparison mutated original snapshot");
            std::cout<<"PASS: "<<name<<" snapshots preserve timestep/properties; paired mitigation counts and unchanged baseline\n";
        }
        return 0;
    } catch (const std::exception& error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}

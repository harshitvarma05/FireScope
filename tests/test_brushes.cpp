#include "firescope/BackendFactory.hpp"
#include "firescope/Scenario.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace firescope;
int main() {
    try {
        for (const auto& name : availableBackends()) {
            SimulationConfig config{19,17}; auto initial = generateScenario(config,42);
            auto backend = createBackend(name); backend->initialize(config,initial);
            EnvironmentParams params; params.baseIgnitionProbability = 0;
            backend->step(params,3);
            backend->applyBrush(BrushTool::Firebreak,9,8,2);
            backend->applyBrush(BrushTool::Vegetation,9,8,1,0.35f);
            GridData grid; backend->downloadGrid(grid);
            const size_t center = 8*19+9, outside = 8*19+11;
            if (grid.state[center]!=1 || grid.fuel[center]!=0.35f || grid.density[center]!=0.35f ||
                grid.vegetation[center]!=0.35f || grid.state[outside]!=0 || grid.burnTicks[center]!=0 || backend->stats().timestep!=3)
                throw std::runtime_error("Vegetation did not restore cleared forest with the requested properties");
            backend->applyBrush(BrushTool::Water,9,8,0,2);
            backend->applyBrush(BrushTool::Ignite,9,8,0);
            backend->step(params,2);
            GridData before; backend->downloadGrid(before);
            backend->applyBrush(BrushTool::Vegetation,9,8,0,0);
            backend->downloadGrid(grid);
            if (grid.moisture[center]!=1 || grid.state[center]!=2 || grid.fuel[center]!=before.fuel[center] ||
                grid.density[center]!=0.35f || grid.burnTicks[center]!=before.burnTicks[center])
                throw std::runtime_error("Vegetation modified burning cells or field values were lost across buffer swaps");
            backend->applyBrush(BrushTool::Vegetation,0,0,1,0);
            backend->downloadGrid(grid);
            if (grid.state[0]!=0 || grid.fuel[0]!=0 || grid.density[0]!=0 || grid.vegetation[0]!=0)
                throw std::runtime_error("Zero density did not clear unburned forest at the boundary");
            backend->applyBrush(BrushTool::Vegetation,0,0,0,2);
            backend->downloadGrid(grid);
            if (grid.state[0]!=1 || grid.density[0]!=1 || grid.fuel[0]!=1 || grid.vegetation[0]!=1)
                throw std::runtime_error("Vegetation strength did not clamp to one");
            backend->step(params,30); backend->downloadGrid(before);
            if (before.state[center]!=3) throw std::runtime_error("Burning test cell did not exhaust its fuel");
            backend->applyBrush(BrushTool::Vegetation,9,8,0,1); backend->downloadGrid(grid);
            if (grid.state[center]!=3 || grid.fuel[center]!=before.fuel[center])
                throw std::runtime_error("Vegetation silently regrew a burned cell");
            std::cout << "PASS: " << name << " vegetation regrowth/clearing, brush boundaries, water saturation and evolving fields\n";
        }
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

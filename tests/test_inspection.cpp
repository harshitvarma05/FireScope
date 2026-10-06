#include "firescope/CellInspection.hpp"
#include "firescope/DeterministicRNG.hpp"
#include "CPUBackend.hpp"
#include "firescope/Scenario.hpp"
#include <iostream>
#include <stdexcept>
using namespace firescope;
int main() {
    try {
        SimulationConfig config{23,19}; auto grid = generateScenario(config,42);
        CPUBackend cpu; cpu.initialize(config,grid); cpu.applyBrush(BrushTool::Ignite,11,9,2);
        cpu.downloadGrid(grid);
        EnvironmentParams params; params.windSpeed = 7; params.windDirectionRadians = 1.2f;
        std::vector<float> probabilities(grid.state.size());
        for (uint32_t y = 0; y < config.height; ++y) for (uint32_t x = 0; x < config.width; ++x) {
            const auto probe = inspectIgnition(grid,config,params,x,y);
            if (probe.burningNeighbors > 8 || probe.probability < 0 || probe.probability > 1)
                throw std::runtime_error("Invalid probe bounds");
            probabilities[y*config.width+x] = probe.probability;
        }
        cpu.step(params); GridData next; cpu.downloadGrid(next);
        for (uint32_t y = 0; y < config.height; ++y) for (uint32_t x = 0; x < config.width; ++x) {
            const size_t i = y*config.width+x;
            if (grid.state[i] != 1) continue;
            const bool predicted = ignitionSucceeds(hashCell(params.seed,x,y,0),probabilities[i]);
            if ((next.state[i] == 2) != predicted) throw std::runtime_error("Inspector disagrees with CPU update");
        }
        auto isolated = inspectIgnition(grid,config,params,0,0);
        if (isolated.burningNeighbors || isolated.probability) throw std::runtime_error("Isolated cell reports ignition");
        grid.state[9*config.width+11] = 3;
        if (inspectIgnition(grid,config,params,11,9).probability != 0) throw std::runtime_error("Burned cell reports ignition");
        std::cout << "PASS: inspector probabilities predict the CPU transition with wind/moisture/slope\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

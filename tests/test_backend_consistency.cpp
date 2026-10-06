#include "firescope/BackendFactory.hpp"
#include "firescope/Scenario.hpp"
#include <iostream>
#include <stdexcept>
using namespace firescope;
namespace {
void compare(ISimulationBackend& cpu,ISimulationBackend& gpu,const char* label) {
    RenderState a,b; cpu.downloadRenderState(a); gpu.downloadRenderState(b);
    if (a.cellState!=b.cellState || a.fireIntensity!=b.fireIntensity)
        throw std::runtime_error(std::string(label)+": state/intensity mismatch with CPU");
    auto sa=cpu.stats(),sb=gpu.stats();
    if (sa.burned!=sb.burned || sa.burning!=sb.burning || sa.noFuel!=sb.noFuel ||
        sa.unburned!=sb.unburned || sa.timestep!=sb.timestep)
        throw std::runtime_error(std::string(label)+": statistics mismatch");
}
}
int main() {
    try {
        auto available=availableBackends();
        if (available.size()==1) { std::cout<<"SKIP: no GPU backend/device available\n"; return 77; }
        for (const auto& name : available) {
            if (name=="cpu") continue;
            auto cpu=createBackend("cpu"),gpu=createBackend(name);
            // Odd dimensions exercise partial edge threadgroups/blocks.
            SimulationConfig config{19,17}; auto grid=generateScenario(config,42);
            grid.elevation.assign(grid.state.size(),0); grid.moisture.assign(grid.state.size(),0);
            grid.vegetation.assign(grid.state.size(),0.5f); grid.density.assign(grid.state.size(),0.5f);
            EnvironmentParams p; p.windSpeed=0; p.globalHumidity=0; p.moistureStrength=0;
            p.slopeStrength=0; p.baseIgnitionProbability=1;
            for (int scenario=0;scenario<4;++scenario) {
                cpu->initialize(config,grid); gpu->initialize(config,grid);
                if (scenario) {
                    cpu->applyBrush(BrushTool::Ignite,3,8,0); gpu->applyBrush(BrushTool::Ignite,3,8,0);
                }
                if (scenario==2) {
                    for (int y=0;y<17;++y) { cpu->applyBrush(BrushTool::Firebreak,9,y,0); gpu->applyBrush(BrushTool::Firebreak,9,y,0); }
                }
                p.baseIgnitionProbability=scenario==3 ? 0 : 1;
                for (int step=0;step<30;++step) { cpu->step(p); gpu->step(p); compare(*cpu,*gpu,"deterministic scenario"); }
                if (scenario==2 && gpu->stats().unburned!=153) throw std::runtime_error("Firebreak containment failed on GPU");
            }
            config={128,128}; grid=generateScenario(config,42); p=EnvironmentParams{};
            cpu->initialize(config,grid); gpu->initialize(config,grid);
            cpu->applyBrush(BrushTool::Ignite,64,64,2); gpu->applyBrush(BrushTool::Ignite,64,64,2);
            cpu->applyBrush(BrushTool::Water,70,64,5,0.75f); gpu->applyBrush(BrushTool::Water,70,64,5,0.75f);
            for (int step=0;step<40;++step) { cpu->step(p); gpu->step(p); compare(*cpu,*gpu,"seeded wind/moisture/slope"); }
            RenderState first; gpu->downloadRenderState(first);
            gpu->reset(grid); gpu->applyBrush(BrushTool::Ignite,64,64,2); gpu->applyBrush(BrushTool::Water,70,64,5,0.75f);
            gpu->step(p,40); RenderState reset; gpu->downloadRenderState(reset);
            if (first.cellState!=reset.cellState || first.fireIntensity!=reset.fireIntensity)
                throw std::runtime_error("GPU reset/batching failed");
            config={512,512}; grid=generateScenario(config,42);
            gpu->initialize(config,grid); gpu->applyBrush(BrushTool::Ignite,256,256,2); gpu->step(p,25);
            auto stats=gpu->stats();
            if (stats.burning+stats.burned<=13 || stats.noFuel+stats.unburned+stats.burning+stats.burned!=512*512)
                throw std::runtime_error("512 squared GPU smoke run failed");
            std::cout<<"PASS: "<<gpu->name()<<" on "<<gpu->deviceName()<<": exact CPU state/intensity consistency, reset, brushes, partial groups, 512 squared run\n";
        }
        return 0;
    } catch (const std::exception& error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}

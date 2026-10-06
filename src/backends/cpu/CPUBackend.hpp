#pragma once
#include "firescope/SimulationBackend.hpp"
namespace firescope {
class CPUBackend final : public ISimulationBackend {
public:
    const char* name() const override { return "CPU"; }
    void initialize(const SimulationConfig&, const InitialGrid&) override;
    void reset(const InitialGrid&) override;
    void step(const EnvironmentParams&, uint32_t stepCount = 1) override;
    void downloadRenderState(RenderState&) override;
    void downloadGrid(GridData& out) override { out = current_; }
    void restoreSnapshot(const InitialGrid& grid, uint64_t timestep) override { reset(grid); stats_.timestep = timestep; }
    SimulationStats stats() const override { return stats_; }
    void applyBrush(BrushTool, int x, int y, int radius, float strength = 0.25f) override;
    const GridData& grid() const { return current_; }
private:
    void countStates();
    SimulationConfig config_;
    GridData current_, next_;
    SimulationStats stats_;
    bool initialized_ = false;
};
}

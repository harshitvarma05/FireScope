#pragma once
#include "firescope/SimulationBackend.hpp"
#include <memory>
namespace firescope {
class CUDABackend final : public ISimulationBackend {
public:
    CUDABackend();
    ~CUDABackend() override;
    static bool available();
    const char* name() const override { return "CUDA"; }
    std::string deviceName() const override;
    void initialize(const SimulationConfig&, const InitialGrid&) override;
    void reset(const InitialGrid&) override;
    void step(const EnvironmentParams&, uint32_t stepCount = 1) override;
    void downloadRenderState(RenderState&) override;
    void downloadGrid(GridData&) override;
    void restoreSnapshot(const InitialGrid&, uint64_t timestep) override;
    SimulationStats stats() const override;
    void applyBrush(BrushTool, int, int, int, float strength = 0.25f) override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}

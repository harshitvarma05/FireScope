#pragma once
#include "firescope/SimulationBackend.hpp"
#include <memory>
namespace firescope {
class MetalBackend final : public ISimulationBackend {
public:
    MetalBackend();
    ~MetalBackend() override;
    static bool available();
    const char* name() const override { return "Metal"; }
    std::string deviceName() const override;
    void initialize(const SimulationConfig&, const InitialGrid&) override;
    void reset(const InitialGrid&) override;
    void step(const EnvironmentParams&, uint32_t stepCount = 1) override;
    void downloadRenderState(RenderState&) override;
    SimulationStats stats() const override;
    void applyBrush(BrushTool, int, int, int, float strength = 0.25f) override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}

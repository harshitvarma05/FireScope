#pragma once
#include "SimulationTypes.hpp"
namespace firescope {
class ISimulationBackend {
public:
    virtual ~ISimulationBackend() = default;
    virtual const char* name() const = 0;
    virtual void initialize(const SimulationConfig&, const InitialGrid&) = 0;
    virtual void reset(const InitialGrid&) = 0;
    virtual void step(const EnvironmentParams&, uint32_t stepCount = 1) = 0;
    virtual void downloadRenderState(RenderState&) = 0;
    virtual SimulationStats stats() const = 0;
    virtual void applyBrush(BrushTool, int x, int y, int radius, float strength = 0.25f) = 0;
};
}

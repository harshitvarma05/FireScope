#pragma once
#include "SimulationTypes.hpp"
#include <string>
namespace firescope {
class ISimulationBackend {
public:
    virtual ~ISimulationBackend() = default;
    virtual const char* name() const = 0;
    virtual std::string deviceName() const { return "Host CPU"; }
    virtual void initialize(const SimulationConfig&, const InitialGrid&) = 0;
    virtual void reset(const InitialGrid&) = 0;
    virtual void step(const EnvironmentParams&, uint32_t stepCount = 1) = 0;
    virtual void downloadRenderState(RenderState&) = 0;
    virtual void downloadGrid(GridData&) = 0;
    virtual void restoreSnapshot(const InitialGrid&, uint64_t timestep) = 0;
    virtual SimulationStats stats() const = 0;
    virtual void applyBrush(BrushTool, int x, int y, int radius, float strength = 0.25f) = 0;
};
}

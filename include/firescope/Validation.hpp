#pragma once
#include "SimulationTypes.hpp"
#include <cmath>
#include <stdexcept>
namespace firescope {
inline void validateEnvironment(const EnvironmentParams& p) {
    const float values[] = {p.baseIgnitionProbability,p.windSpeed,p.windDirectionRadians,
        p.globalHumidity,p.moistureStrength,p.slopeStrength,p.windStrength,
        p.windAlignmentStrength,p.fuelConsumptionRate,p.cellSizeMeters};
    for (float value : values) if (!std::isfinite(value))
        throw std::invalid_argument("Environment parameters must be finite");
    if (p.cellSizeMeters<=0 || p.fuelConsumptionRate<0 || p.windSpeed<0 ||
        p.baseIgnitionProbability<0 || p.baseIgnitionProbability>1 ||
        p.globalHumidity<0 || p.globalHumidity>1 || p.moistureStrength<0 ||
        p.windStrength<0 || p.windAlignmentStrength<0)
        throw std::invalid_argument("Environment parameter out of range");
}
}

#pragma once
#include "SimulationTypes.hpp"
namespace firescope {
int runGui(const SimulationConfig&, const EnvironmentParams&, bool smokeTest);
int runTriangle(bool smokeTest);
}

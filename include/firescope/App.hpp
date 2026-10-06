#pragma once
#include "SimulationTypes.hpp"
#include <string>
namespace firescope {
int runGui(const SimulationConfig&, const EnvironmentParams&, bool smokeTest, const std::string& backendName);
int runTriangle(bool smokeTest);
}

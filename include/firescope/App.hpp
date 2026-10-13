#pragma once
#include "SimulationTypes.hpp"
#include <string>
namespace firescope {
int runGui(const SimulationConfig&, const EnvironmentParams&, bool smokeTest, const std::string& backendName, bool showcase = false);
int runTriangle(bool smokeTest);
}

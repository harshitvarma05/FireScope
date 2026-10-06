#pragma once
#include "SimulationBackend.hpp"
#include <memory>
#include <string>
#include <vector>
namespace firescope {
std::vector<std::string> availableBackends();
std::unique_ptr<ISimulationBackend> createBackend(const std::string& name);
}

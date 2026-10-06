#include "firescope/BackendFactory.hpp"
#include "CPUBackend.hpp"
#ifdef FIRESCOPE_HAS_CUDA
#include "CUDABackend.hpp"
#endif
#ifdef FIRESCOPE_HAS_METAL
#include "MetalBackend.hpp"
#endif
#include <stdexcept>
namespace firescope {
std::vector<std::string> availableBackends() {
    std::vector<std::string> result{"cpu"};
#ifdef FIRESCOPE_HAS_CUDA
    if (CUDABackend::available()) result.emplace_back("cuda");
#endif
#ifdef FIRESCOPE_HAS_METAL
    if (MetalBackend::available()) result.emplace_back("metal");
#endif
    return result;
}
std::unique_ptr<ISimulationBackend> createBackend(const std::string& name) {
    if (name == "cpu") return std::make_unique<CPUBackend>();
#ifdef FIRESCOPE_HAS_CUDA
    if (name == "cuda" && CUDABackend::available()) return std::make_unique<CUDABackend>();
#endif
#ifdef FIRESCOPE_HAS_METAL
    if (name == "metal" && MetalBackend::available()) return std::make_unique<MetalBackend>();
#endif
    throw std::runtime_error("Backend unavailable: " + name + ". Use --list-backends to check this build/device.");
}
}

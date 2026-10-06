#include "firescope/App.hpp"
#include "firescope/Scenario.hpp"
#include "CPUBackend.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
uint32_t number(const std::string& text, uint32_t maximum) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Expected an unsigned integer: " + text);
    auto n = std::stoull(text);
    if (n > maximum) throw std::invalid_argument("Value exceeds allowed range: " + text);
    return static_cast<uint32_t>(n);
}
}
int main(int argc, char** argv) {
    using namespace firescope;
    try {
        SimulationConfig config;
        EnvironmentParams params;
        uint32_t steps = 500;
        bool headless = false, smoke = false, triangle = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            auto value = [&]() -> std::string {
                if (++i >= argc) throw std::invalid_argument("Missing value for " + arg);
                return argv[i];
            };
            if (arg == "--headless") headless = true;
            else if (arg == "--smoke-test") smoke = true;
            else if (arg == "--triangle") triangle = true;
            else if (arg == "--backend") {
                if (value() != "cpu") throw std::invalid_argument("Only CPU is implemented at this milestone");
            } else if (arg == "--grid") config.width = config.height = number(value(), 2048);
            else if (arg == "--seed") params.seed = number(value(), UINT32_MAX);
            else if (arg == "--steps") steps = number(value(), 1000000);
            else if (arg == "--help") {
                std::cout << "FireScope [--backend cpu] [--grid 1..2048] [--seed N] [--steps N]\n"
                             "          [--headless] [--triangle] [--smoke-test]\n";
                return 0;
            } else throw std::invalid_argument("Unknown option: " + arg);
        }
        if (!config.width) throw std::invalid_argument("Grid must be nonzero");
        if (headless) {
            CPUBackend backend;
            backend.initialize(config, generateScenario(config, params.seed));
            backend.applyBrush(BrushTool::Ignite, config.width / 2, config.height / 2, 2);
            auto begin = std::chrono::steady_clock::now();
            backend.step(params, steps);
            auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
            auto stats = backend.stats();
            std::cout << "backend,device,grid,steps,seed,total_ms,ms_per_step,mcell_updates_per_s,burned_final\n";
            std::cout << std::fixed << std::setprecision(6) << "cpu,host," << config.width << ',' << steps << ','
                      << params.seed << ',' << ms << ',' << (steps ? ms / steps : 0) << ','
                      << (ms > 0 ? static_cast<double>(config.width) * config.height * steps / (ms * 1000) : 0)
                      << ',' << stats.burned << '\n';
            return 0;
        }
#ifdef FIRESCOPE_BUILD_GUI
        return triangle ? runTriangle(smoke) : runGui(config, params, smoke);
#else
        throw std::invalid_argument("GUI was disabled at build time; use --headless");
#endif
    } catch (const std::exception& error) {
        std::cerr << "FireScope: " << error.what() << '\n';
        return 1;
    }
}

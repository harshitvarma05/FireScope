#include "CPUBackend.hpp"
#include "firescope/Scenario.hpp"
#include <iostream>
#include <stdexcept>
using namespace firescope;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
InitialGrid flat(int n) {
    InitialGrid g; g.state.assign(n,1); g.fuel.assign(n,1); g.moisture.assign(n,0);
    g.elevation.assign(n,0); g.vegetation.assign(n,0.5f); g.density.assign(n,0.5f); g.burnTicks.assign(n,0);
    return g;
}
EnvironmentParams certain() {
    EnvironmentParams p; p.baseIgnitionProbability = 1; p.windSpeed = 0;
    p.globalHumidity = 0; p.moistureStrength = 0; p.slopeStrength = 0; p.minimumBurnTicks = 1;
    return p;
}
void report(const char* text) { std::cout << "PASS: " << text << '\n'; }
}
int main() {
    try {
        CPUBackend cpu;
        auto g = flat(25); auto p = certain();
        cpu.initialize({5,5},g); cpu.step(p,20);
        require(cpu.grid().state == g.state && cpu.grid().fuel == g.fuel,"No ignition changed grid");
        report("no burning cells means no ignition");

        cpu.initialize({5,5},g); cpu.applyBrush(BrushTool::Ignite,2,2,0); cpu.step(p);
        require(cpu.stats().burning == 9,"Probability one must ignite all eight neighbors");
        require(cpu.grid().state[0] == 1,"In-place update propagated multiple cells in one step");
        cpu.step(p); require(cpu.stats().burning == 25,"Second ring did not ignite");
        report("probability one, Moore neighborhood and synchronous double buffering");

        cpu.initialize({5,5},g); cpu.applyBrush(BrushTool::Ignite,0,0,0); cpu.step(p);
        require(cpu.stats().burning == 4 && cpu.grid().state[24] == 1,"Boundary wrapped or read out of bounds");
        report("corner boundaries do not wrap");

        cpu.initialize({7,5},flat(35));
        for (int y = 0; y < 5; ++y) cpu.applyBrush(BrushTool::Firebreak,3,y,0);
        cpu.applyBrush(BrushTool::Ignite,1,2,0); cpu.step(p,40);
        require(cpu.stats().burned == 15 && cpu.stats().noFuel == 5 && cpu.stats().unburned == 15,
                "Complete firebreak did not stop fire");
        report("complete no-fuel barrier stops fire");

        auto burned = flat(1); burned.state[0] = 2;
        cpu.initialize({1,1},burned); p.fuelConsumptionRate = 1; p.minimumBurnTicks = 3;
        cpu.step(p,2); require(cpu.stats().burning == 1 && cpu.grid().fuel[0] == 0,"Minimum burn time failed");
        cpu.step(p); require(cpu.stats().burned == 1,"Fuel exhaustion transition failed");
        cpu.step(p,10); require(cpu.stats().burned == 1,"Burned cell reignited");
        report("fuel exhaustion, minimum burn ticks and permanent burned state");

        p = EnvironmentParams{};
        auto scenario = generateScenario({128,128},42);
        cpu.initialize({128,128},scenario); cpu.applyBrush(BrushTool::Ignite,64,64,2); cpu.step(p,40);
        auto first = cpu.grid();
        auto stats = cpu.stats();
        require(stats.burning + stats.burned > 13,"128 squared model did not spread");
        require(stats.noFuel + stats.unburned + stats.burning + stats.burned == 128*128,"Stats lost cells");
        cpu.reset(scenario); require(cpu.stats().timestep == 0,"Reset did not reset timestep");
        cpu.applyBrush(BrushTool::Ignite,64,64,2); cpu.step(p,40);
        require(cpu.grid().state == first.state && cpu.grid().fuel == first.fuel &&
                cpu.grid().burnTicks == first.burnTicks,"Seeded reset not reproducible");
        CPUBackend batched; batched.initialize({128,128},scenario);
        batched.applyBrush(BrushTool::Ignite,64,64,2);
        for (int i = 0; i < 40; ++i) batched.step(p);
        require(batched.grid().state == first.state,"Batched and individual steps differ");
        report("128 squared spread, counts, seeded reset and step batching");

        // Paired one-step trials isolate directional and moisture factors from
        // later stochastic feedback. Both trials use identical random hashes.
        int downwind = 0, upwind = 0, uphill = 0, downhill = 0, dry = 0, wet = 0;
        for (uint32_t seed = 0; seed < 1000; ++seed) {
            auto small = flat(9); small.state[4] = 2;
            p = certain(); p.seed = seed; p.baseIgnitionProbability = 0.3f; p.windSpeed = 20;
            cpu.initialize({3,3},small); cpu.step(p);
            downwind += cpu.grid().state[5] == 2; upwind += cpu.grid().state[3] == 2;
            p.windSpeed = 0; p.slopeStrength = 3;
            small.elevation[5] = 20; small.elevation[3] = -20;
            cpu.initialize({3,3},small); cpu.step(p);
            uphill += cpu.grid().state[5] == 2; downhill += cpu.grid().state[3] == 2;
            small.elevation.assign(9,0); p.slopeStrength = 0; p.moistureStrength = 6;
            cpu.initialize({3,3},small); cpu.step(p); auto dryGrid = cpu.grid();
            dry += static_cast<int>(cpu.stats().burning);
            cpu.initialize({3,3},small); cpu.applyBrush(BrushTool::Water,1,1,2,1); cpu.step(p);
            wet += static_cast<int>(cpu.stats().burning);
            for (size_t i = 0; i < 9; ++i)
                require(cpu.grid().state[i] != 2 || dryGrid.state[i] == 2,"Wet trial ignited where paired dry trial did not");
        }
        require(downwind > upwind * 3,"Wind did not bias spread toward the wind direction");
        require(uphill > downhill * 3,"Slope did not favor uphill spread");
        require(dry > wet * 2,"Moisture did not suppress spread");
        report("wind direction, uphill spread and moisture suppression across paired seeds");

        bool rejected = false;
        try { cpu.initialize({2,2},flat(3)); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected,"Malformed grid accepted");
        rejected = false; p.cellSizeMeters = 0;
        try { cpu.step(p); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected,"Zero cell size accepted");
        report("invalid grid and environment parameters rejected");
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}

/* C++ Headers */
#include <iostream>
#include <string>
#include <atomic>

/* Metal includes and defines */
#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

/* Header files */
#include "MyAppDelegate/MyAppDelegate.hpp"
#include "SimulationEngine/SimulationEngine.hpp"

// ── Compute subprocess mode ───────────────────────────────────────────────────
//
// Reads config from CLI flags, runs simulation, writes one readout float
// per line to stdout. The Go server reads these line-by-line for progress.
// Exit 0 = completed, non-zero = error.

static void runCompute(const SimulationConfig& cfg) {
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    if (!device) {
        std::cerr << "No Metal device found\n";
        std::exit(1);
    }

    SimulationEngine engine(device, cfg);
    std::atomic<bool> cancelled{false};

    engine.run([](uint32_t step, float readout) {
        std::cout << readout << '\n';
        // Periodic flush so the Go server receives values while the sim runs
        if ((step & 0xFF) == 0xFF) std::cout.flush();
    }, cancelled);

    std::cout.flush();
    device->release();
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, const char* argv[]) {
    bool gui = false;
    SimulationConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--gui") {
            gui = true;
        } else if (arg == "--excitation" && i + 1 < argc) {
            cfg.excitationType = argv[++i];
        } else if (arg == "--steps" && i + 1 < argc) {
            cfg.totalSteps = static_cast<uint32_t>(std::stoul(argv[++i]));
        }
    }

    if (gui) {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        MyAppDelegate app;
        NS::Application* pApp = NS::Application::sharedApplication();
        pApp->setDelegate(&app);
        pApp->run();
        pool->release();
    } else {
        runCompute(cfg);
    }

    return 0;
}

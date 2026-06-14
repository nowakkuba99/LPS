#pragma once
#include <string>
#include <cstdint>

struct SimulationConfig {
    std::string excitationType = "wave_mix"; // "single_sin" | "wave_mix" | "lg"
    uint32_t    totalSteps     = 30000;
};

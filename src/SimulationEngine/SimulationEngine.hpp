#pragma once

#include <Metal/Metal.hpp>
#include <functional>
#include <atomic>
#include <memory>
#include <vector>
#include <cstdint>

#include "SimulationConfig.hpp"

class SimulationEngine {
public:
    SimulationEngine(MTL::Device* device, const SimulationConfig& config);
    ~SimulationEngine();

    // Runs synchronously on the calling thread.
    // progressCb(step, readout) is called after each completed step.
    // Returns true if all steps completed, false if cancelled.
    bool run(std::function<void(uint32_t step, float readout)> progressCb,
             std::atomic<bool>& cancelled);

private:
    struct FrameData {
        uint32_t counter;
        float    excitation;
        float    readOut;
    };

    static constexpr uint32_t kWidth  = 8002;
    static constexpr uint32_t kHeight = 22;

    SimulationConfig _config;

    MTL::Device*               _pDevice;      // non-owning — shared with JobManager
    MTL::CommandQueue*         _pCommandQueue;
    MTL::ComputePipelineState* _pComputePSO;
    MTL::Texture*              _pTexture;
    MTL::Buffer*               _pFrameDataBuffer;

    std::unique_ptr<std::vector<float>> _pExcitation;

    void buildComputePipeline();
    void buildTexture();
    void buildBuffer();
};

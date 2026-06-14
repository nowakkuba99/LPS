#include "SimulationEngine.hpp"
#include "FileReader/FileReader.hpp"
#include "MathHelpers/SolverHelpers.hpp"

#include <cassert>
#include <iostream>

static const std::string kComputeShaderPath = "/shaders/computeCrackElastic.metal";

SimulationEngine::SimulationEngine(MTL::Device* device, const SimulationConfig& config)
    : _config(config), _pDevice(device)
{
    _pCommandQueue = _pDevice->newCommandQueue();

    if (config.excitationType == "single_sin")
        _pExcitation = getExcitationSingleSin();
    else if (config.excitationType == "lg")
        _pExcitation = getExcitationLG();
    else
        _pExcitation = getExcitationWaveMix();

    buildComputePipeline();
    buildTexture();
    buildBuffer();
}

SimulationEngine::~SimulationEngine() {
    _pFrameDataBuffer->release();
    _pTexture->release();
    _pComputePSO->release();
    _pCommandQueue->release();
    // _pDevice is not released — owned by JobManager
}

void SimulationEngine::buildComputePipeline() {
    using NS::StringEncoding::UTF8StringEncoding;

    std::string src = read_file(kComputeShaderPath);
    NS::Error* err  = nullptr;

    auto* lib = _pDevice->newLibrary(
        NS::String::string(src.c_str(), UTF8StringEncoding), nullptr, &err);
    if (!lib) {
        __builtin_printf("Shader compile error: %s\n",
                         err->localizedDescription()->utf8String());
        assert(false);
    }

    auto* fn = lib->newFunction(NS::String::string("computeLISA", UTF8StringEncoding));
    _pComputePSO = _pDevice->newComputePipelineState(fn, &err);
    if (!_pComputePSO) {
        __builtin_printf("Pipeline error: %s\n",
                         err->localizedDescription()->utf8String());
        assert(false);
    }

    fn->release();
    lib->release();
}

void SimulationEngine::buildTexture() {
    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setWidth(kWidth);
    desc->setHeight(kHeight);
    desc->setPixelFormat(MTL::PixelFormatRGBA32Float);
    desc->setTextureType(MTL::TextureType2D);
    desc->setStorageMode(MTL::StorageMode::StorageModeManaged);
    desc->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);

    _pTexture = _pDevice->newTexture(desc);
    desc->release();

    std::vector<float> zeros(kWidth * kHeight * 4, 0.0f);
    _pTexture->replaceRegion(
        MTL::Region(0, 0, 0, kWidth, kHeight, 1),
        0, zeros.data(),
        kWidth * 4 * sizeof(float));
}

void SimulationEngine::buildBuffer() {
    _pFrameDataBuffer = _pDevice->newBuffer(sizeof(FrameData),
                                            MTL::ResourceStorageModeManaged);
}

bool SimulationEngine::run(std::function<void(uint32_t, float)> progressCb,
                           std::atomic<bool>& cancelled)
{
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    auto excitIt = _pExcitation->begin();
    NS::UInteger maxThreads = _pComputePSO->maxTotalThreadsPerThreadgroup();

    for (uint32_t step = 0; step < _config.totalSteps; ++step) {
        if (cancelled) {
            pool->release();
            return false;
        }

        // Write frame data for this step
        auto* fd = reinterpret_cast<FrameData*>(_pFrameDataBuffer->contents());
        fd->counter    = step % 4;
        fd->excitation = (excitIt != _pExcitation->end()) ? *excitIt : 0.0f;
        fd->readOut    = 0.0f;
        _pFrameDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(FrameData)));

        // Encode and dispatch compute kernel
        auto* cmdBuf  = _pCommandQueue->commandBuffer();
        auto* encoder = cmdBuf->computeCommandEncoder();
        encoder->setComputePipelineState(_pComputePSO);
        encoder->setTexture(_pTexture, 0);
        encoder->setBuffer(_pFrameDataBuffer, 0, 0);
        encoder->dispatchThreads(
            MTL::Size(kWidth, kHeight, 1),
            MTL::Size(maxThreads, 1, 1));
        encoder->endEncoding();
        cmdBuf->commit();
        // Wait so readOut is fully written before the CPU reads it
        cmdBuf->waitUntilCompleted();

        float readout = reinterpret_cast<FrameData*>(_pFrameDataBuffer->contents())->readOut;
        progressCb(step, readout);

        if (excitIt != _pExcitation->end()) ++excitIt;
    }

    pool->release();
    return true;
}

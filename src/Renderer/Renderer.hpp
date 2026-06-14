#ifndef FD3DC257_AB48_4D15_B6A4_3EA50BE9B700
#define FD3DC257_AB48_4D15_B6A4_3EA50BE9B700

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include <vector>
#include <algorithm>
#include <simd/simd.h>
#include <memory>
#include <chrono>
#include <ctime>
#include <fstream>

#include "FileReader/FileReader.hpp"
#include "FileReader/FileOperations.hpp"
#include "MathHelpers/SolverHelpers.hpp"


class Renderer
{
public:
    Renderer( MTL::Device* pDevice ) :  _pDevice( pDevice->retain()),
                                        _pCommandQueue(_pDevice->newCommandQueue()),
                                        _frame(0),
                                        _semaphoreTexture(dispatch_semaphore_create(Renderer::kMaxFramesInFlight)),
                                        _start(std::chrono::system_clock::now()),
                                        _c(0),
                                        _fw("readOut.dat")
    {
        _pTextureDataBuffer.reserve(kMaxFramesInFlight);
        for(int i = 0; i<kMaxFramesInFlight; ++i) {
            _pTextureDataBuffer.push_back(nullptr);
            _pDisplayDataBuffer.push_back(nullptr);
        }

//        _pExcitation = getExcitationSingleSin();
        _pExcitation = getExcitationWaveMix();
//        _pExcitation = getExcitationLG();
        _iExcitation = _pExcitation->begin();

        buildShaders();
        buildComputePipeline();
        buildBuffers();
        buildTextures();
    }

    ~Renderer() {
        _pDevice->release();
        _pCommandQueue->release();
        _pShaderLibrary->release();
        _pPSO->release();
        _pComputePSO->release();
        _pVertexDataBuffer->release();
        for(auto& b : _pTextureDataBuffer) {
            b->release();
        }
        for(auto& b : _pDisplayDataBuffer) {
            b->release();
        }
        _pTexture->release();
    }

    void buildShaders();
    void buildComputePipeline();
    void buildBuffers();
    void buildTextures();
    void compute();
    void readOut();
    void draw( MTK::View* pView );

private:
    MTL::Device*                    _pDevice;
    MTL::CommandQueue*              _pCommandQueue;
    MTL::Library*                   _pShaderLibrary;
    MTL::RenderPipelineState*       _pPSO;
    MTL::ComputePipelineState*      _pComputePSO;

    MTL::Buffer*                    _pVertexDataBuffer;
    std::vector<MTL::Buffer*>       _pTextureDataBuffer;
    std::vector<MTL::Buffer*>       _pDisplayDataBuffer;
    MTL::Texture*                   _pTexture;

    int                     _frame;
    dispatch_semaphore_t    _semaphoreTexture;

    std::unique_ptr<std::vector<float>> _pExcitation;
    std::vector<float>::iterator        _iExcitation;

    std::chrono::time_point<std::chrono::system_clock> _start;
    uint _c;
    FileWriter _fw;

    struct VertexData {
        simd::float3 position;
        simd::float2 textureCoord;
    };

    struct FrameData {
        simd::uint1 counter;
        simd::float1 excitation;
        simd::float1 readOut;
    };

    static const int kMaxFramesInFlight {4};
    static constexpr uint32_t kTextureWidth  {8002};
    static constexpr uint32_t kTextureHeight {22};
};

#endif /* FD3DC257_AB48_4D15_B6A4_3EA50BE9B700 */

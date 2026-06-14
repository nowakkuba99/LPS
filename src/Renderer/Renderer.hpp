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


/*
    Renderer class which has the draw() method called by Metal API each 60s
 
    It handles the building shaders and performing draw calls
 */
class Renderer
{
public:
    Renderer( MTL::Device* pDevice ) :  _pDevice( pDevice->retain()),
                                        _pCommandQueue(_pDevice->newCommandQueue()),
                                        _frame(0),
                                        _semaphoreTexture(dispatch_semaphore_create(Renderer::kMaxFramesInFlight)),
                                        _semaphoreDisplay(dispatch_semaphore_create(Renderer::kMaxFramesInFlight)),
                                        _start(std::chrono::system_clock::now()),
                                        _c(0),
                                        _fw("readOut.dat")
    {
        // Prepare TextureDataBuffer
        _pTextureDataBuffer.reserve(kMaxFramesInFlight);
        for(int i = 0; i<kMaxFramesInFlight; ++i) {
            _pTextureDataBuffer.push_back(nullptr);
            _pDisplayDataBuffer.push_back(nullptr);
        }
        
        // Prepare Extortion
//        _pExtortion = getExtortionSingleSin();
        _pExtortion = getExtortionWaveMix();
//        _pExtortion = getExtortionLG();
        _iExtortion = _pExtortion->begin();
        
        // Renderer actions
        buildShaders();
        buildComputePipeline();
        buildBuffers();
        buildTextures();
    }
    
    // Custom destructor used to release objects that we own
    ~Renderer() {
        // Metal handlers release
        _pDevice->release();
        _pCommandQueue->release();
        _pShaderLibrary->release();
        _pPSO->release();
        _pComputePSO->release();
        // Data containers release
        _pVertexDataBuffer->release();
        for(auto& b : _pTextureDataBuffer) {
            b->release();
        }
        for(auto& b : _pDisplayDataBuffer) {
            b->release();
        }
        _pTexture->release();
    }

    void buildShaders();                                    // Prepares vertex and fragment shader
    void buildComputePipeline();                            // Prepares compute shader and creates compute pipeline state
    void buildBuffers();                                    // Preapres and fills _pVertexDataBuffer
    void buildTextures();                                   // Preapres and fills _pTexture
    void compute(MTL::CommandBuffer* pCommandBuffer);       // When called performs single computation on _pTexture
    void compute();                                         // Performs computation with new Command Buffer Object
    void readOut();
    void draw( MTK::View* pView );                          // Function called FPS times per second - used to display _pTexture onto the screen

private:
    // Metal handlers
    MTL::Device*                    _pDevice;               // GPU handle
    MTL::CommandQueue*              _pCommandQueue;         // Queue where GPU commands are encoded
    MTL::Library*                   _pShaderLibrary;        // Shader library is an object for compiling all shader codes (compute + display)
    MTL::RenderPipelineState*       _pPSO;                  // Pipeline state object used to describe the configuration of render pipeline
    MTL::ComputePipelineState*      _pComputePSO;           // Pipeline state object used to describe the configuration of compute pipeline
    
    // Data containers
    // Buffers
    MTL::Buffer*                    _pVertexDataBuffer;
    std::vector<MTL::Buffer*>       _pTextureDataBuffer;
    std::vector<MTL::Buffer*>       _pDisplayDataBuffer;
    // Textures
    MTL::Texture*                   _pTexture;
        
    
    // Used to syncrhonize TextureDataBuffer read/write
    int                     _frame;
    dispatch_semaphore_t    _semaphoreTexture, _semaphoreDisplay;
    
    // Solver params -> Maybe to be extracted
    // Extortion
    std::unique_ptr<std::vector<float>> _pExtortion;        // Holds the extortion data that's generated somewhere else :)
    std::vector<float>::iterator     _iExtortion;           // Iterator to use proper extortion at proper time frame
    
    // Measure performance
    std::chrono::time_point<std::chrono::system_clock> _start;
    // Time count
    uint _c;
    // Save results to file
    FileWriter _fw;

    
    // Cusotom structs for handling CPU <-> GPU data transfers
    struct VertexData {                                     // Used to describe the contents of '_pVertexDataBuffer'
        simd::float3 position;
        simd::float2 textureCoord;
    };
    
    struct FrameData {                                      // Used to describe the contents of '_pTextureDataBuffer'
        simd::uint1 counter;
        simd::float1 extortion;
        simd::float1 readOut;
    };
    
    struct DisplayFrameData {
        simd::uint1 counter;
    };
    
    // Constant parameters
    static const int kMaxFramesInFlight {4};    // Used to avoid data race
    static constexpr uint32_t   kTextureWidth {8002}; //8002
    static constexpr uint32_t   kTextureHeight {22}; //22
};

#endif /* FD3DC257_AB48_4D15_B6A4_3EA50BE9B700 */

#include "Renderer.hpp"

#include <simd/simd.h>  // this makes vscode intellisense stop working :(
#include <iostream>
#include <cstdlib>


const static std::string SHADER_PATH = "/shaders/triangle.metal";   // shader path relative to executable placement
//const static std::string COMPUTE_SHADER_PATH = "/shaders/computeCrack.metal";
const static std::string COMPUTE_SHADER_PATH = "/shaders/computeCrackElastic.metal";

void Renderer::buildShaders() {
    using NS::StringEncoding::UTF8StringEncoding;
    auto shaderSrc = read_file(SHADER_PATH);    // Read shader code
    
    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary(NS::String::string(shaderSrc.c_str(), UTF8StringEncoding), nullptr, &pError);
    if (!pLibrary)
    {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }
    /*
        Vertex shader specifies how the GPU should transform vericies of the triangle
     */
    MTL::Function* pVertexFn = pLibrary->newFunction(NS::String::string("vertexMain", UTF8StringEncoding));
    
    /*
        Fragment shader specifies the final color of triangle's pixels
     */
    MTL::Function* pFragFn = pLibrary->newFunction(NS::String::string("fragmentMain", UTF8StringEncoding));
    
    // Create a Render Pipeline Descriptor to designate the two shaders the pipeline should use
    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(pVertexFn);
    pDesc->setFragmentFunction(pFragFn);
    pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

    /*
        Render Pipeline State objects are expensive to create since Metal invokes a compiler to convert
        shader code to GPU code - this should only be done once: either at startup or when starting some opration and load is expected
     */
    _pPSO = _pDevice->newRenderPipelineState(pDesc, &pError);
    if (!_pPSO)
    {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    pVertexFn->release();
    pFragFn->release();
    pDesc->release();
    _pShaderLibrary = pLibrary; // Will be released with destructor of Renderer
}

void Renderer::buildComputePipeline() {
    std::string shaderSrc = read_file(COMPUTE_SHADER_PATH);
    
    NS::Error* pError {nullptr};
    MTL::Library* pComputeLibrary = _pDevice->newLibrary(NS::String::string(shaderSrc.c_str(),NS::UTF8StringEncoding), nullptr, &pError);
    if(!pComputeLibrary) {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }
    
    MTL::Function* pFunction = pComputeLibrary->newFunction(NS::String::string("add",NS::UTF8StringEncoding));
    _pComputePSO = _pDevice->newComputePipelineState(pFunction, &pError);
    if(!_pComputePSO) {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }
    
    pFunction->release();
    pComputeLibrary->release();
}

void Renderer::buildTextures() {

    MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pTextureDesc->setWidth(kTextureWidth);
    pTextureDesc->setHeight(kTextureHeight);
    pTextureDesc->setPixelFormat(MTL::PixelFormatRGBA32Float);
    pTextureDesc->setTextureType(MTL::TextureType2D);
    pTextureDesc->setStorageMode(MTL::StorageMode::StorageModeManaged);
    pTextureDesc->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    
    MTL::Texture* pTexture = _pDevice->newTexture(pTextureDesc);
    _pTexture = pTexture;
    float32_t* textureData = new float32_t[kTextureWidth*kTextureHeight*sizeof(float32_t)];
    for ( size_t y = 0; y < kTextureHeight; ++y )
    {
        for ( size_t x = 0; x < kTextureWidth; ++x )
        {
            size_t i = y * kTextureWidth + x;
                textureData[ i * 4 + 0 ] = 0;
                textureData[ i * 4 + 1 ] = 0;
                textureData[ i * 4 + 2 ] = 0;
                textureData[ i * 4 + 3 ] = 0;
        }
    }
    // copy the texture data to texture object - this has to be called as well for custom textures
    // Its expensive to create a texture so only at startup!
    _pTexture->replaceRegion( MTL::Region( 0, 0, 0, kTextureWidth, kTextureHeight, 1 ), 0, textureData, kTextureWidth * 4 * sizeof(float32_t) );

pTextureDesc->release();
}

void Renderer::buildBuffers() {
    //  -- VertexDataBuffer --
    // Create data
    const float s = 0.8f;
    VertexData verticies[] =
    {
        //                             Texture
        //        Positions          Coordinates
        {{   s,       s,     0.0f }, {1.f, 0.f}},
        {{   s,      -s,     0.0f }, {1.f, 1.f}},
        {{  -s,       s,     0.0f }, {0.f, 0.f}},
        
        {{  -s,       s,     0.0f }, {0.f, 0.f}},
        {{  -s,      -s,     0.0f }, {0.f, 1.f}},
        {{   s,      -s,     0.0f }, {1.f, 1.f}}
    };
    const size_t vertexDataSize = sizeof(verticies);
    // Create buffer and copy data into it
    _pVertexDataBuffer = _pDevice->newBuffer(vertexDataSize, MTL::ResourceStorageModeManaged);  // MTL::ResourceStorageModeManaged - accesible by both GPU and CPU
    memcpy(_pVertexDataBuffer->contents(), verticies, vertexDataSize);
    
    // Indicate to Metal that the CPU has written data to the buffer
    _pVertexDataBuffer->didModifyRange(NS::Range::Make(0, _pVertexDataBuffer->length()));
    
    // -- TextureDataBuffer --
    for(auto& buff : _pTextureDataBuffer) {
        buff = _pDevice->newBuffer(sizeof(FrameData), MTL::ResourceStorageModeManaged);
    }
    
    // -- DisplayDataBuffer --
    for(auto& buff : _pDisplayDataBuffer) {
        buff = _pDevice->newBuffer(sizeof(FrameData), MTL::ResourceStorageModeManaged);
    }
}


void Renderer::compute(MTL::CommandBuffer* pCommandBuffer) {
    assert(pCommandBuffer);

    MTL::ComputeCommandEncoder* pComputeEncoder = pCommandBuffer->computeCommandEncoder();
    
    pComputeEncoder->setComputePipelineState(_pComputePSO);
    pComputeEncoder->setTexture(_pTexture, 0);
    pComputeEncoder->setBuffer(_pTextureDataBuffer.at(_frame),0,0);
    
    MTL::Size gridSize = MTL::Size(kTextureWidth,kTextureHeight,1);
    
    NS::UInteger maxThreadGroupSize = _pComputePSO->maxTotalThreadsPerThreadgroup();
    MTL::Size threadGroupSize = MTL::Size(maxThreadGroupSize,1,1);
    
    pComputeEncoder->dispatchThreads(gridSize, threadGroupSize);
    
    pComputeEncoder->endEncoding();
}

void Renderer::compute() {
    MTL::CommandBuffer* pNewCommandBuffer = _pCommandQueue->commandBuffer();
//    // Read data from frame and write to file
//    const auto readOut {static_cast<float>(reinterpret_cast<FrameData*>(_pTextureDataBuffer.at(_frame)->contents())->readOut)};
//    _fw.writeFloat(readOut,++cccc,_frame);
    // Calculate the current frame to write to
    _frame = (_frame + 1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* pTextureDataBuffer = _pTextureDataBuffer.at(_frame);
    // Synchronize current frame with GPU
    dispatch_semaphore_wait(_semaphoreTexture, DISPATCH_TIME_FOREVER);
    pNewCommandBuffer->addCompletedHandler([this](MTL::CommandBuffer* pCmd){
        dispatch_semaphore_signal(_semaphoreTexture);
    });
    
    // Set the current counter and extortion values
    FrameData* pFrameData = reinterpret_cast<FrameData*>(pTextureDataBuffer->contents());
    pFrameData->counter = _frame;
    pFrameData->extortion = *_iExtortion;
    pTextureDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(FrameData)));
    
    // Try fast computing
    MTL::ComputeCommandEncoder* pComputeEncoder = pNewCommandBuffer->computeCommandEncoder();

    pComputeEncoder->setComputePipelineState(_pComputePSO);
    pComputeEncoder->setTexture(_pTexture, 0);
    pComputeEncoder->setBuffer(_pTextureDataBuffer.at(_frame),0,0);

    MTL::Size gridSize = MTL::Size(kTextureWidth,kTextureHeight,1);

    NS::UInteger maxThreadGroupSize = _pComputePSO->maxTotalThreadsPerThreadgroup();
    MTL::Size threadGroupSize = MTL::Size(maxThreadGroupSize,1,1);

    pComputeEncoder->dispatchThreads(gridSize, threadGroupSize);

    pComputeEncoder->endEncoding();
    
    pNewCommandBuffer->commit();
    ++_iExtortion;
}

void Renderer::readOut() {
    // Read data from frame and write to file
    const auto readOut {static_cast<float>(reinterpret_cast<FrameData*>(_pTextureDataBuffer.at(_frame)->contents())->readOut)};
    _fw.writeFloat(readOut);
}

const int limit = 30000;
void Renderer::draw( MTK::View* pView ) {
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
    if(_c<limit){
        for(int i = 0; i<150; ++i){ //150
            compute();
            readOut();
            if(_c>=limit) {
                auto end = std::chrono::system_clock::now();
                
                std::chrono::duration<double> elapsed_seconds = end-_start;
                std::time_t end_time = std::chrono::system_clock::to_time_t(end);
                
                std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "s"
                << std::endl;
//                std::this_thread::sleep_for(std::chrono::seconds(2));
                break;
            }
            ++_c;
        }
    }
    // Create a command buffer object. This allows the app to encode commands for execution by the GPU.
    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    
    // Synchronize current frame with GPU
    dispatch_semaphore_wait(_semaphoreTexture, DISPATCH_TIME_FOREVER);
    pCmd->addCompletedHandler([this](MTL::CommandBuffer* pCmd){
        dispatch_semaphore_signal(_semaphoreTexture);
    });
    
    // Create a render command encoder object.
    // This prepares the command buffer to receive drawing commands and specifies the actions to perform when drawing starts and ends.
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

    // Encode commands the the GPU to draw triangle
    pEnc->setRenderPipelineState( _pPSO );
    // Set buffers as parameters to shader code
    pEnc->setVertexBuffer(_pVertexDataBuffer, 0, 0);
    
    // Set FrameDataBuffer index to 1
    MTL::Buffer* pDisplayDataBuffer = _pDisplayDataBuffer.at(_frame);
    pEnc->setVertexBuffer(pDisplayDataBuffer, 0, 1);
    
    // Set texture
    pEnc->setFragmentTexture(_pTexture, /*index*/0);
    
    // Draw rectangle from verticies in the render pipeline
    pEnc->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6), NS::UInteger(2));


    pEnc->endEncoding();
    // Present the current drawable. This encodes a command to make the results of the GPU’s work visible on the screen
    pCmd->presentDrawable( pView->currentDrawable() );
    // Submit the command buffer to its command queue. This submits the encoded commands to the GPU for execution.
    pCmd->commit();

    pPool->release();
}

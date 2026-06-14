#ifndef DDA5B58C_76B2_4382_ADC4_C0D2B68BC39E
#define DDA5B58C_76B2_4382_ADC4_C0D2B68BC39E

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include "Renderer/Renderer.hpp"

// Provides an interface where MTK::View can forward events
// By overriding functions from base class can respond to events
// Calls drawInMTKView every frame
class MyMTKViewDelegate : public MTK::ViewDelegate {
public:
    MyMTKViewDelegate(MTL::Device* device) : MTK::ViewDelegate() {
        _pRenderer = std::make_unique<Renderer>(device);
    }
    virtual ~MyMTKViewDelegate() = default;
    virtual void drawInMTKView(MTK::View* view) override;
private:
    std::unique_ptr<Renderer> _pRenderer;
};

#endif /* DDA5B58C_76B2_4382_ADC4_C0D2B68BC39E */

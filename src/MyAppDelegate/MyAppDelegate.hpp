#ifndef CD4228BE_241B_4C01_83BF_CDED349884AE
#define CD4228BE_241B_4C01_83BF_CDED349884AE

#include <memory>

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include "MyMTKViewDelegate/MyMTKViewDelegate.hpp"

class MyAppDelegate : public NS::ApplicationDelegate
{
    public:
        ~MyAppDelegate() {
            _pMtkView->release();
            _pWindow->release();
            _pDevice->release();
        }

        virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override;
        virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override;
        virtual bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;

    private:
        // Those free are pointers to objects created for us so we can not delete those - we need to release
        NS::Window* _pWindow;
        MTK::View* _pMtkView;
        MTL::Device* _pDevice;
        // All pointers of our own classes can be smart pointers
        std::unique_ptr<MyMTKViewDelegate> _pViewDelegate;
};

#endif /* CD4228BE_241B_4C01_83BF_CDED349884AE */

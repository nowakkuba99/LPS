#include "MyAppDelegate.hpp"

/*
    This is an overwrite of method that will be called when app will finish launching
*/
void MyAppDelegate::applicationWillFinishLaunching( NS::Notification* pNotification ) {
    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
}

/*
    This is called when app finished writing. It's basically a setup before after than
    the app loop is started.
*/
void MyAppDelegate::applicationDidFinishLaunching( NS::Notification* pNotification ) {
    // Prepare the content to show in the window
    // Frame with size
    CGRect frame = (CGRect){ {100.0, 100.0}, {1024.0, 1024.0} };
    // Create new window
    _pWindow = NS::Window::alloc()->init(
        frame,
        NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled,
        NS::BackingStoreBuffered,
        false );
    // The device is a wrapper over system GPU
    _pDevice = MTL::CreateSystemDefaultDevice();
    // MTKView provides a Metal-aware view that you can use to render graphics using Metal and display them onscreen
    _pMtkView = MTK::View::alloc()->init( frame, _pDevice );
    _pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    _pMtkView->setClearColor( MTL::ClearColor::Make( 0.1, 0.1, 0.1, 1.0 ) );
//    _pMtkView->setDepthStencilPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);
    // MyMTKViewDelegate is a subclass of the MTK::ViewDelegate class. It provides an interface to which the MTK::View can forward events.
    // By overriding the virtual functions of its parent class, MyMTKViewDelegate can respond to these events. 
    // MTK::View calls the drawInMTKView() method each frame allowing the app to update any rendering
    _pViewDelegate = std::make_unique<MyMTKViewDelegate>( _pDevice );
    _pMtkView->setDelegate( _pViewDelegate.get() ); // setting the delegate is equak to specifing where to use the view
    // Set the View inside the window
    _pWindow->setContentView( _pMtkView );
    _pWindow->setTitle( NS::String::string( "LISA SH Waves Solver", NS::StringEncoding::UTF8StringEncoding ) );

    _pWindow->makeKeyAndOrderFront( nullptr );

    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->activateIgnoringOtherApps( true );
}

bool MyAppDelegate::applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) {
    return true;
}

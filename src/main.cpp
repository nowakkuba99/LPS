/* C++ Headers */
#include <iostream>
#include <memory>

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

/*
    LISA SH Solver for linear specimen
 */


int main(int argc, const char * argv[]) {
    // Create auto release pool object to allow auto releasing
    NS::AutoreleasePool* pAutoReleasePool {NS::AutoreleasePool::alloc()->init()};
    // Get globabl shared application object
    MyAppDelegate app;
    NS::Application* pSharedApplication {NS::Application::sharedApplication()};
    pSharedApplication->setDelegate(&app);
    pSharedApplication->run();
    pAutoReleasePool->release();
    
    return 0;
}

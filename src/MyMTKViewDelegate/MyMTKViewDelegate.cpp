#include "MyMTKViewDelegate.hpp"

// Simply calls the Renderer class's draw() method.
void MyMTKViewDelegate::drawInMTKView( MTK::View* pView )
{
//    pView->setPreferredFramesPerSecond(1);
    _pRenderer->draw( pView );
}

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "Renderer.h"
#include "Config.h"

extern "C" void* createWindow(int width, int height, const char* title);
extern "C" void* getMetalLayer(void* window);
extern "C" void showWindow(void* window);
extern "C" void runApp();
extern Renderer*      gRenderer;
extern CA::MetalLayer* gLayer;


int main(int argc, char *argv[]) {
    NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();
    
    // Create window using C interface
    void* window = createWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    void* metalLayer = getMetalLayer(window);
    
    // Create Metal device and renderer
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    auto* renderer = new Renderer(device);
    
    // Cast the layer to CA::MetalLayer and render
    auto* layer = static_cast<CA::MetalLayer*>(metalLayer);
    layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    layer->setFramebufferOnly(true);

    gRenderer = renderer;
    gLayer =  static_cast<CA::MetalLayer*>(metalLayer);

    // Show window and run app
    showWindow(window);
    runApp();
    
    // Cleanup
    delete renderer;
    pAutoreleasePool->release();
    
    return 0;
}
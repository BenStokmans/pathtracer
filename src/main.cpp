#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <atomic>
#include <thread>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "Renderer.h"
#include "Config.h"

extern "C" void *createWindow(int width, int height, const char *title);

extern "C" void *getMetalLayer(void *window);

extern "C" void showWindow(void *window);

extern "C" void runApp();

extern Renderer *gRenderer;
extern CA::MetalLayer *gLayer;
extern MovementHandler *gMovement;

int main(int argc, char *argv[]) {
    NS::AutoreleasePool *pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

    // Create window using C interface
    void *window = createWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    void *metalLayer = getMetalLayer(window);

    // Create Metal device and renderer
    MTL::Device *device = MTL::CreateSystemDefaultDevice();
    auto *renderer = new Renderer(device);
    gMovement = &renderer->movement();

    // Cast the layer to CA::MetalLayer and render
    auto *layer = static_cast<CA::MetalLayer *>(metalLayer);
    layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    layer->setFramebufferOnly(true);

    gRenderer = renderer;
    gLayer = static_cast<CA::MetalLayer *>(metalLayer);

    std::atomic running{true};
    std::thread mover([&]() {
        using clock = std::chrono::high_resolution_clock;
        auto last = clock::now();
        while (running.load()) {
            auto now = clock::now();
            float dt = std::chrono::duration<float>(now - last).count();
            last = now;

            gMovement->update(dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // ~200 Hz
        }
    });

    // Show window and run app
    showWindow(window);
    runApp();

    // Cleanup
    running = false;
    mover.join();
    delete renderer;
    pAutoreleasePool->release();

    return 0;
}

#pragma once
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

class Renderer {
public:
    Renderer(MTL::Device *device);
    ~Renderer();
    void draw(CA::MetalLayer* layer);
    
private:
    MTL::Device *_device;
    MTL::CommandQueue *_cmdQueue;
    MTL::RenderPipelineState *_pipelineState;
    MTL::Buffer *_vertexBuffer;
};
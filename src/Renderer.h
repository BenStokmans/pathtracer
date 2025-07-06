#pragma once
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

class Renderer {
public:
    explicit Renderer(MTL::Device *device);
    void draw(CA::MetalLayer *layer);

private:
    MTL::Device *_device;
    MTL::CommandQueue *_cmdQueue;
    MTL::Texture           *_outputTexture;
    MTL::ComputePipelineState  *_computePipeline;
    MTL::RenderPipelineState   *_quadPipeline;
    MTL::SamplerState          *_quadSampler;

    MTL::Buffer *_triangleBuffer;
    uint32_t _triangleCount;
    MTL::Buffer *_planeBuffer;
    uint32_t _planeCount;
    MTL::Buffer *_sphereBuffer;
    uint32_t _sphereCount;
    MTL::Buffer *_materialBuffer;
    uint32_t _materialCount;

    uint32_t _frameIndex = 0;
    void setupPipeline();
    void setupTexture();
    void setupScene();
};
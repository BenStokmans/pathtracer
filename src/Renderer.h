#pragma once
#include <Metal/Metal.hpp>
#import <QuartzCore/QuartzCore.hpp>
#include "Config.h"

class Renderer {
public:
    Renderer(MTL::Device *device);
    void draw(CA::MetalLayer *layer);

private:
    MTL::Device *_device;
    MTL::CommandQueue *_cmdQueue;
    MTL::Texture           *_outputTexture;
    MTL::ComputePipelineState  *_computePipeline;
    MTL::RenderPipelineState   *_quadPipeline;
    MTL::SamplerState          *_quadSampler;

    MTL::Buffer           *_sceneBuffer;
    uint32_t               _objectCount;

    uint32_t _frameIndex = 0;
    void setupPipeline();
    void setupTexture();
    void setupScene();
};
#pragma once
#include <chrono>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <simd/vector_types.h>

#include "MovementHandler.h"

class Renderer {
public:
    explicit Renderer(MTL::Device *device);

    void draw(CA::MetalLayer *layer);

    void resetCamera();

    void keyEvent(uint16_t key);

    void mouseEvent(double dx, double dy);

    MovementHandler &movement() { return _move; }

private:
    MTL::Device *_device;
    MTL::CommandQueue *_cmdQueue;
    MTL::Texture *_outputTexture;
    MTL::ComputePipelineState *_computePipeline;
    MTL::RenderPipelineState *_quadPipeline;
    MTL::SamplerState *_quadSampler;

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

    void clearAccumulation();

    simd::float3 _camPos = {0, 1, 3};
    float _yaw = 0.0f; // in radians
    float _pitch = 0.0f;
    float _fov = 45.0f; // degrees
    simd::float3 _velocity = {0, 0, 0};

    MovementHandler _move;
    std::chrono::high_resolution_clock::time_point _lastUpdate;


    std::chrono::high_resolution_clock::time_point _lastFpsTime;
    uint32_t _framesSinceLastFps = 0;
};

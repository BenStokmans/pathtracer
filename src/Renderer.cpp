#include "Renderer.h"
#include "Primitives/ScenePrimitives.h"

#include <iostream>
#include "Config.h"
#include <vector>

#include "Camera.h"
#include "Material.h"

static constexpr float PI_F = 3.14159265358979323846f; // Define PI constant

Renderer::Renderer(MTL::Device *device) : _device(device) {
    _cmdQueue = _device->newCommandQueue();
    _lastFpsTime = std::chrono::high_resolution_clock::now();
    _lastUpdate = std::chrono::high_resolution_clock::now();
    setupPipeline();
    setupOutputTexture();
    setupScene();
}

void Renderer::setupPipeline() {
    auto lib = _device->newDefaultLibrary();
    // compute pipeline
    auto comp = lib->newFunction(NS::String::string("path_trace", NS::UTF8StringEncoding));
    NS::Error *error = nullptr;
    _computePipeline = _device->newComputePipelineState(comp, &error);

    // display pipeline (fullscreen quad)
    auto vfn = lib->newFunction(NS::String::string("quad_vert", NS::UTF8StringEncoding));
    auto ffn = lib->newFunction(NS::String::string("quad_frag", NS::UTF8StringEncoding));
    auto pd = MTL::RenderPipelineDescriptor::alloc()->init();
    pd->setVertexFunction(vfn);
    pd->setFragmentFunction(ffn);
    pd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    _quadPipeline = _device->newRenderPipelineState(pd, &error);

    // sampler for the quad pass
    auto sd = MTL::SamplerDescriptor::alloc()->init();
    sd->setMinFilter(MTL::SamplerMinMagFilterNearest);
    sd->setMagFilter(MTL::SamplerMinMagFilterNearest);
    _quadSampler = _device->newSamplerState(sd);
}

void Renderer::setupOutputTexture() {
    auto desc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatRGBA32Float,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        false
    );
    desc->setUsage(MTL::TextureUsageShaderWrite | MTL::TextureUsageShaderRead);
    _outputTexture = _device->newTexture(desc);
    auto cmdBuf = _cmdQueue->commandBuffer();
    auto blit = cmdBuf->blitCommandEncoder();
    MTL::Region full = MTL::Region::Make2D(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    std::vector<float> zero(WINDOW_WIDTH * WINDOW_HEIGHT * 4, 0.0f);
    _outputTexture->replaceRegion(
        full, 0, zero.data(), WINDOW_WIDTH * 4 * sizeof(float)
    );
    blit->endEncoding();
    cmdBuf->commit();
    cmdBuf->waitUntilCompleted();
}


void Renderer::draw(CA::MetalLayer *layer) {
    // compute delta‐time
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - _lastUpdate).count();
    _lastUpdate = now;

    // update camera
    _move.update(dt);
    _camPos = _move.position();
    _yaw = _move.yaw();
    _pitch = _move.pitch();

    if (_move.hasMoved()) {
        clearAccumulation();
        _move.resetMoved();
    }

    // reset accumulation if moving
    if (simd::length(_move.position() - _camPos) > 1e-5f)
        clearAccumulation();

    // get a drawable for this frame
    auto drawable = layer->nextDrawable();
    if (!drawable) return;

    auto cmdBuf = _cmdQueue->commandBuffer();
    auto encoder = cmdBuf->computeCommandEncoder();
    encoder->setComputePipelineState(_computePipeline);
    encoder->setTexture(_outputTexture, 0);
    // bind triangles
    encoder->setBuffer(_triangleBuffer, 0, 1);
    encoder->setBytes(&_triangleCount, sizeof(_triangleCount), 2);
    // bind planes
    encoder->setBuffer(_planeBuffer, 0, 3);
    encoder->setBytes(&_planeCount, sizeof(_planeCount), 4);
    // bind spheres
    encoder->setBuffer(_sphereBuffer, 0, 5);
    encoder->setBytes(&_sphereCount, sizeof(_sphereCount), 6);

    encoder->setBytes(&_frameIndex, sizeof(_frameIndex), 7);

    encoder->setBuffer(_materialBuffer, 0, 8);
    encoder->setBytes(&_materialCount, sizeof(_materialCount), 9);

    float aspect = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    float theta = _fov * (PI_F / 180.0f);
    float halfH = tan(theta * 0.5f);
    float halfW = aspect * halfH;

    simd::float3 front = {
        cos(_pitch) * sin(_yaw),
        sin(_pitch),
        cos(_pitch) * cos(_yaw)
    };
    simd::float3 worldUp = simd_make_float3(0.0f, 1.0f, 0.0f);
    simd::float3 right = simd::normalize(simd::cross(front, worldUp));
    simd::float3 up = simd::cross(right, front);

    Camera cam{};
    cam.origin = _camPos;
    cam.lowerLeft = _camPos + front - right * halfW - up * halfH;
    cam.horizontal = 2 * halfW * right;
    cam.vertical = 2 * halfH * up;

    encoder->setBytes(&cam, sizeof(cam), 10);

    MTL::Size threadsPerThreadgroup(8, 8, 1);
    MTL::Size grid(WINDOW_WIDTH, WINDOW_HEIGHT, 1);
    MTL::Size threadgroups(
        (grid.width + threadsPerThreadgroup.width - 1) / threadsPerThreadgroup.width,
        (grid.height + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height,
        1
    );
    encoder->dispatchThreadgroups(threadgroups, threadsPerThreadgroup);
    encoder->endEncoding();

    auto rpd = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto att = rpd->colorAttachments()->object(0);
    att->setTexture(drawable->texture());
    att->setLoadAction(MTL::LoadActionClear);
    att->setStoreAction(MTL::StoreActionStore);

    auto re = cmdBuf->renderCommandEncoder(rpd);
    re->setRenderPipelineState(_quadPipeline);
    re->setFragmentTexture(_outputTexture, 0);
    re->setFragmentSamplerState(_quadSampler, 0);
    // draw two triangles as a strip
    re->drawPrimitives(
        MTL::PrimitiveTypeTriangleStrip,
        static_cast<NS::UInteger>(0),
        4
    );
    re->endEncoding();

    cmdBuf->presentDrawable(drawable);
    cmdBuf->commit();

    _framesSinceLastFps++;

    // check if one second has elapsed
    now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double>(now - _lastFpsTime).count();
    if (elapsed >= 1.0) {
        double fps = static_cast<double>(_framesSinceLastFps) / elapsed;
        std::cout << "FPS: " << fps << std::endl;
        // reset
        _framesSinceLastFps = 0;
        _lastFpsTime = now;
    }

    bool inCooldown = (now - _move.lastInteraction) < std::chrono::milliseconds(100);
    if (!inCooldown) {
        _frameIndex++;
    }

    _frameIndex++;
}

void Renderer::setupScene() {
    //
    //  1) MATERIALS
    //
    //  idx 0: emissive “light panel”
    //  idx 1: white diffuse (walls, floor, back)
    //  idx 2: red diffuse
    //  idx 3: green diffuse
    //  idx 4: mirror
    //  idx 5: glass (ior=1.5)
    std::vector<Material> mats = {
        // albedo         emission        reflectivity  ior
        {{0, 0, 0}, {15, 15, 15}, 0.0f, 1.0f}, // 0 light
        {{0.8f, 0.8f, 0.8f}, {0, 0, 0}, 0.0f, 1.0f}, // 1 white
        {{0.8f, 0.2f, 0.2f}, {0, 0, 0}, 0.0f, 1.0f}, // 2 red
        {{0.2f, 0.8f, 0.2f}, {0, 0, 0}, 0.0f, 1.0f}, // 3 green
        {{0.9f, 0.9f, 0.9f}, {0, 0, 0}, 1.0f, 1.0f}, // 4 mirror
        {{1.0f, 1.0f, 1.0f}, {0, 0, 0}, 0.0f, 1.5f} // 5 glass
    };
    _materialCount = (uint32_t) mats.size();
    _materialBuffer = _device->newBuffer(
        mats.size() * sizeof(Material),
        MTL::ResourceStorageModeShared
    );
    memcpy(_materialBuffer->contents(), mats.data(), mats.size() * sizeof(Material));

    //
    //  2) GEOMETRY
    //
    //  a) Ceiling “light panel” as two triangles (mat 0)
    using Tri = SceneTriangle;
    std::vector<Tri> tris;
    float yL = 1.9f; // just below the ceiling
    float x0 = -0.5f, x1 = 0.5f;
    float z0 = -2.0f, z1 = -1.0f;
    tris.push_back({{x0, yL, z0}, {x1, yL, z0}, {x1, yL, z1}, 0});
    tris.push_back({{x1, yL, z1}, {x0, yL, z1}, {x0, yL, z0}, 0});

    //  b) Spheres (mat 2:red, 4:mirror, 5:glass, 3:green)
    using Sph = SceneSphere;
    std::vector<Sph> sphs = {
        {{-0.6f, 0.25f, -0.1f}, 0.25f, 2}, // small red
        {{0.0f, 0.25f, -0.2f}, 0.25f, 4}, // mirror
        {{0.6f, 0.25f, -0.3f}, 0.25f, 5}, // glass
        {{0.0f, 0.9f, -0.2f}, 0.25f, 3} // green
    };

    //  c) Walls & floor & back (infinite planes, mat 1)
    using Pln = ScenePlane;
    std::vector<Pln> plns = {
        // normal         d        matIndex
        {{0, 1, 0}, 0.0f, 1}, // floor y=0
        {{0, -1, 0}, 2.0f, 1}, // ceiling y=2
        {{1, 0, 0}, 2.0f, 1}, // left  x=-2
        {{-1, 0, 0}, 2.0f, 1}, // right x= 2
        {{0, 0, 1}, 3.0f, 1} // back  z=-3
    };

    // upload
    _triangleCount = (uint32_t) tris.size();
    _planeCount = (uint32_t) plns.size();
    _sphereCount = (uint32_t) sphs.size();

    _triangleBuffer = _device->newBuffer(
        tris.size() * sizeof(Tri),
        MTL::ResourceStorageModeShared
    );
    memcpy(_triangleBuffer->contents(), tris.data(), tris.size() * sizeof(Tri));

    _planeBuffer = _device->newBuffer(
        plns.size() * sizeof(Pln),
        MTL::ResourceStorageModeShared
    );
    memcpy(_planeBuffer->contents(), plns.data(), plns.size() * sizeof(Pln));

    _sphereBuffer = _device->newBuffer(
        sphs.size() * sizeof(Sph),
        MTL::ResourceStorageModeShared
    );
    memcpy(_sphereBuffer->contents(), sphs.data(), sphs.size() * sizeof(Sph));
}

void Renderer::resetCamera() {
    _camPos = {0, 1, 3};
    _yaw = 3.14159265f;
    _pitch = 0.0f;
}

void Renderer::clearAccumulation() {
    // reset our sample counter
    _frameIndex = 0;
}

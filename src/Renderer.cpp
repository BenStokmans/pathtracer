#include "Renderer.h"
#include "Primitives/ScenePrimitives.h"

#include <iostream>
#include "Config.h"
#include <vector>
#include "Material.h"

Renderer::Renderer(MTL::Device *device) : _device(device) {
    _cmdQueue = _device->newCommandQueue();
    setupPipeline();
    setupTexture();
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

void Renderer::setupTexture() {
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

    _frameIndex++;
}

void Renderer::setupScene() {
    std::vector<Material> mats;
    // a bright emissive sky‐light object (if you want geometry lights later)
    mats.push_back({{0, 0, 0}, {5, 5, 5}, 0.0f});

    // triangle
    mats.push_back({
        {0.8f, 0.2f, 0.2f},
        {0, 0, 0},
        0.0f
    });

    //plane
    mats.push_back({
        {0.2f, 0.8f, 0.2f},
        {0, 0, 0},
        0.0f
    });

    // sphere
    mats.push_back({
        {0.2f, 0.2f, 0.8f},
        {0, 0, 0},
        0.8f
    });

    _materialCount = static_cast<uint32_t>(mats.size());
    _materialBuffer = _device->newBuffer(
        mats.size() * sizeof(Material),
        MTL::ResourceStorageModeShared
    );
    memcpy(_materialBuffer->contents(), mats.data(), mats.size() * sizeof(Material));

    using Tri = SceneTriangle;
    using Pln = ScenePlane;
    using Sph = SceneSphere;

    std::vector<SceneTriangle> tris = {
        {{-0.5f, 0, -1}, {0.5f, 0, -1}, {0, 1, -1}, 1}
    };
    std::vector<ScenePlane> plns = {
        {{0, 1, 0}, 1.0f, 2}
    };
    std::vector<SceneSphere> sphs = {
        {{0.0f, 10.0f, 0.0f}, 5.0f, 0},
        {{0.5f, 0.5f, -0.5f}, 0.5f, 3},
        {{-0.5f, 0.5f, -0.5f}, 0.5f, 3}
    };

    _triangleCount = tris.size();
    _planeCount = plns.size();
    _sphereCount = sphs.size();

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

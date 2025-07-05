#include "Renderer.h"
#include "Config.h"

Renderer::Renderer(MTL::Device *device) : _device(device) {
    _cmdQueue = _device->newCommandQueue();
    setupPipeline();
    setupTexture();
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
    auto pd  = MTL::RenderPipelineDescriptor::alloc()->init();
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
    auto blit   = cmdBuf->blitCommandEncoder();
    MTL::Region full = MTL::Region::Make2D(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
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
    encoder->setBytes(&_frameIndex, sizeof(_frameIndex), 0);

    MTL::Size threadsPerThreadgroup(8,8,1);
    MTL::Size grid(WINDOW_WIDTH, WINDOW_HEIGHT,1);
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
        static_cast<NS::UInteger>(4)
    );    re->endEncoding();

    cmdBuf->presentDrawable(drawable);
    cmdBuf->commit();

    _frameIndex++;
}
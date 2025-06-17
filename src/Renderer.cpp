#include "Renderer.h"
#include "Config.h"

Renderer::Renderer(MTL::Device *device) : _device(device) {
    // 3 vertices of a triangle (x,y,z)
    const float vertices[] = {
        0.0f,  0.5f, 0.0f,  // top
       -0.5f, -0.5f, 0.0f,  // bottom left
        0.5f, -0.5f, 0.0f   // bottom right
    };
    _vertexBuffer = _device->newBuffer(vertices, sizeof(vertices), MTL::ResourceStorageModeShared);
    _cmdQueue = _device->newCommandQueue();

    // Load and compile shaders from default library
    MTL::Library *lib = _device->newDefaultLibrary();
    MTL::Function *vs = lib->newFunction(NS::String::string("vertex_main", NS::UTF8StringEncoding));
    MTL::Function *fs = lib->newFunction(NS::String::string("fragment_main", NS::UTF8StringEncoding));
    
    // Create render pipeline descriptor
    MTL::RenderPipelineDescriptor *desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vs);
    desc->setFragmentFunction(fs);
    desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

    NS::Error *error = nullptr;
    _pipelineState = _device->newRenderPipelineState(desc, &error);
    
    // Clean up
    vs->release();
    fs->release();
    lib->release();
    desc->release();
}

Renderer::~Renderer() {
    if (_vertexBuffer) _vertexBuffer->release();
    if (_cmdQueue) _cmdQueue->release();
    if (_pipelineState) _pipelineState->release();
}

void Renderer::draw(CA::MetalLayer* layer) {
    // Get the next drawable
    CA::MetalDrawable* drawable = layer->nextDrawable();
    if (!drawable) return;
    
    // Create render pass descriptor
    MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::alloc()->init();
    rpd->colorAttachments()->object(0)->setTexture(drawable->texture());
    rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
    rpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0));
    rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);
    
    // Create command buffer and render pass
    MTL::CommandBuffer *cmdBuf = _cmdQueue->commandBuffer();
    MTL::RenderCommandEncoder *enc = cmdBuf->renderCommandEncoder(rpd);

    enc->setRenderPipelineState(_pipelineState);
    enc->setVertexBuffer(_vertexBuffer, 0, 0);
    enc->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(TRIANGLE_VERTEX_COUNT));

    enc->endEncoding();
    cmdBuf->presentDrawable(drawable);
    cmdBuf->commit();
    
    // Clean up
    rpd->release();
}
#include <metal_stdlib>
using namespace metal;

// Vertex shader: takes position from buffer(0) and outputs a 4D clip position
vertex float4 vertex_main(const device float3 *pos [[buffer(0)]],
                          uint vid [[vertex_id]])
{
    float3 p = pos[vid];
    return float4(p, 1.0);
}

// Fragment shader: outputs a solid color (e.g. orange)
fragment half4 fragment_main()
{
    return half4(1.0, 0.5, 0.2, 1.0);
}
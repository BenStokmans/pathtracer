#include <metal_stdlib>
using namespace metal;

struct Ray {
    float3 origin;
    float3 dir;
};

// Simple ray‐sphere, returns t or –1 if miss
float hitSphere(float3 center, float radius, Ray r) {
    float3 oc = r.origin - center;
    float a = dot(r.dir, r.dir);
    float b = dot(oc, r.dir);
    float c = dot(oc, oc) - radius*radius;
    float disc = b*b - a*c;
    if (disc < 0.0) return -1.0;
    return (-b - sqrt(disc)) / a;
}

kernel void path_trace(texture2d<float, access::write> outTex [[texture(0)]],
                       constant uint &frameIndex           [[buffer(0)]],
                       uint2 gid                           [[thread_position_in_grid]]) {
    uint W = outTex.get_width();
    uint H = outTex.get_height();
    if (gid.x >= W || gid.y >= H) return;

    // uv in [0,1], flip Y so (0,0) is top-left
    float u = (float(gid.x) + 0.5) / float(W);
    float v = 1.0 - (float(gid.y) + 0.5) / float(H);

    // build primary ray
    float3 origin    = float3(0,1,3);
    float3 lowerLeft = float3(-2.0, -1.5, -1.0);
    float3 horiz     = float3(4.0,  0.0,  0.0);
    float3 vert      = float3(0.0,  3.0,  0.0);
    Ray r = { origin, normalize(lowerLeft + u*horiz + v*vert - origin) };

    // sphere intersection
    float t_sphere = hitSphere(float3(0,0,-1), 0.5, r);

    // plane at y = -1.0
    float t_plane = -1.0;
    if (r.dir.y != 0.0) {
        t_plane = (-1.0 - r.origin.y) / r.dir.y;
        if (t_plane <= 0.0) t_plane = -1.0;
    }

    // shading
    float3 col;
    // define a simple directional light once
    const float3 lightDir = normalize(float3(1,1,1));

    if (t_sphere > 0.0 && (t_plane < 0.0 || t_sphere < t_plane)) {
        // sphere hit
        float3 P = r.origin + t_sphere * r.dir;
        float3 N = normalize(P - float3(0,0,-1));
        float diff = max(dot(N, lightDir), 0.0);
        col = diff * float3(0.7,0.3,0.3);
    }
    else if (t_plane > 0.0) {
        // plane hit
        float3 P = r.origin + t_plane * r.dir;
        // checkerboard: alternate 1/0 by floor(x)+floor(z)
        float c = fmod(floor(P.x) + floor(P.z), 2.0);
        float3 base = (c < 1.0) ? float3(1.0) : float3(0.0);
        float diff = max(dot(float3(0,1,0), lightDir), 0.0);
        col = diff * base;
    }
    else {
        // background gradient
        float3 dirn = normalize(r.dir);
        float t2 = 0.5*(dirn.y + 1.0);
        col = mix(float3(1.0), float3(0.5,0.7,1.0), t2);
    }

    outTex.write(float4(col,1.0), gid);
}

// Vertex→fragment struct
struct VSOut {
    float4 position [[position]];
    float2 uv       [[user(locn0)]];
};

// emit a full‐screen strip plus proper UVs
vertex VSOut quad_vert(uint vid [[vertex_id]]) {
    constexpr float2 pts[4] = { {-1,-1}, { 1,-1}, {-1, 1}, { 1, 1} };
    // we want (0,0) at top-left, (1,1) at bottom-right
    constexpr float2 uvs[4]  = { {0,1}, {1,1}, {0,0}, {1,0} };
    VSOut out;
    out.position = float4(pts[vid], 0, 1);
    out.uv       = uvs[vid];
    return out;
}

// now uv is correct, no flip or magic needed
fragment float4 quad_frag(VSOut in    [[stage_in]],
                          texture2d<float> src [[texture(0)]],
                          sampler           smp [[sampler(0)]]) {
    float3 hdr = src.sample(smp, in.uv).xyz;
    float3 ldr = sqrt(hdr);   // gamma ≈2
    return float4(ldr, 1);
}
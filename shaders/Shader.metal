#include <metal_stdlib>
using namespace metal;

struct SceneTriangle {
    float3 v0;
    float3 v1;
    float3 v2;
};

struct ScenePlane {
    float3 normal;
    float  d;
};

struct SceneSphere {
    float3 center;
    float  radius;
};

struct Ray {
    float3 origin;
    float3 dir;
};

// --- Intersection helpers ---

// Möller–Trumbore ray/triangle: returns t or –1 if no hit
inline float hitTriangle(SceneTriangle tri, Ray r) {
    const float EPS = 1e-6;
    float3 edge1 = tri.v1 - tri.v0;
    float3 edge2 = tri.v2 - tri.v0;
    float3 pvec  = cross(r.dir, edge2);
    float det   = dot(edge1, pvec);
    if (fabs(det) < EPS) return -1.0;
    float invDet = 1.0 / det;
    float3 tvec = r.origin - tri.v0;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0) return -1.0;
    float3 qvec = cross(tvec, edge1);
    float v = dot(r.dir, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0) return -1.0;
    float t = dot(edge2, qvec) * invDet;
    return t > EPS ? t : -1.0;
}

// Plane hit: returns t or –1
inline float hitPlane(ScenePlane pl, Ray r) {
    float denom = dot(pl.normal, r.dir);
    if (fabs(denom) < 1e-6) return -1.0;
    float t = -(dot(pl.normal, r.origin) + pl.d) / denom;
    return t > 0.0 ? t : -1.0;
}

inline float hitSphere(const SceneSphere sph, const Ray r) {
    float3 oc = r.origin - sph.center;
    float  a  = dot(r.dir, r.dir);
    float  b  = dot(oc, r.dir);
    float  c  = dot(oc, oc) - sph.radius * sph.radius;
    float  disc = b*b - a*c;
    if (disc < 0.0) return -1.0;
    float t = (-b - sqrt(disc)) / a;
    return t > 1e-6 ? t : -1.0;
}

// --- Compute kernel ---

kernel void path_trace(
    texture2d<float, access::write> outTex       [[ texture(0) ]],
    device const SceneTriangle      *triangles    [[ buffer(1) ]],
    constant uint                   &triCount     [[ buffer(2) ]],
    device const ScenePlane         *planes       [[ buffer(3) ]],
    constant uint                   &planeCount   [[ buffer(4) ]],
    device const SceneSphere        *spheres      [[ buffer(5) ]],
    constant uint                   &sphereCount  [[ buffer(6) ]],
    uint2                             gid         [[ thread_position_in_grid ]]
) {
    uint W = outTex.get_width(), H = outTex.get_height();
    if (gid.x >= W || gid.y >= H) return;

    // build primary ray
    float u = (float(gid.x) + 0.5) / float(W);
    float v = 1.0 - (float(gid.y) + 0.5) / float(H);
    float3 origin = float3(0,1,3);
    float3 ll     = float3(-2.0, -1.5, -1.0);
    float3 horiz  = float3(4.0,  0.0,  0.0);
    float3 vert   = float3(0.0,  3.0,  0.0);
    Ray   r       = { origin, normalize(ll + u*horiz + v*vert - origin) };

    // Blinn–Phong light setup
    float3 lightDir  = normalize(float3(-1, 1, -0.5));
    float3 viewDir   = normalize(-r.dir);
    float3 ambientLC = float3(0.1);      // ambient coefficient
    float  shininess = 32.0;             // specular exponent

    // find nearest hit & record normal + base color
    float  tMin      = 1e20;
    float3 hitNormal = float3(0);
    float3 baseColor = float3(0,0,0);

    // Triangles → red
    for (uint i = 0; i < triCount; ++i) {
        float t = hitTriangle(triangles[i], r);
        if (t > 0.0 && t < tMin) {
            tMin      = t;
            // compute triangle normal
            float3 e1 = triangles[i].v1 - triangles[i].v0;
            float3 e2 = triangles[i].v2 - triangles[i].v0;
            hitNormal = normalize(cross(e1, e2));
            baseColor = float3(1,0,0);
        }
    }
    // Planes → green
    for (uint i = 0; i < planeCount; ++i) {
        float t = hitPlane(planes[i], r);
        if (t > 0.0 && t < tMin) {
            tMin      = t;
            hitNormal = normalize(planes[i].normal);
            baseColor = float3(0,1,0);
        }
    }
    // Spheres → blue
    for (uint i = 0; i < sphereCount; ++i) {
        float t = hitSphere(spheres[i], r);
        if (t > 0.0 && t < tMin) {
            tMin      = t;
            // normal at hit point
            float3 P   = r.origin + t * r.dir;
            hitNormal  = normalize(P - spheres[i].center);
            baseColor  = float3(0,0,1);
        }
    }

    float3 col;
    if (tMin < 1e19) {
        // compute Blinn–Phong
        float NdotL = max(dot(hitNormal, lightDir), 0.0);
        float3  H   = normalize(lightDir + viewDir);
        float NdotH = max(dot(hitNormal, H), 0.0);
        float3 diffuse = NdotL * baseColor;
        float3 spec    = pow(NdotH, shininess) * float3(1.0); // white highlight

        col = ambientLC * baseColor + diffuse + spec;
    } else {
        // background gradient
        float3 dirn = normalize(r.dir);
        float t2 = 0.5 * (dirn.y + 1.0);
        col = mix(float3(1.0), float3(0.5, 0.7, 1.0), t2);
    }

    outTex.write(float4(col, 1.0), gid);
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

fragment float4 quad_frag(VSOut in    [[stage_in]],
                          texture2d<float> src [[texture(0)]],
                          sampler           smp [[sampler(0)]]) {
    float3 hdr = src.sample(smp, in.uv).xyz;
    float3 ldr = sqrt(hdr);   // gamma ≈2
    return float4(ldr, 1);
}
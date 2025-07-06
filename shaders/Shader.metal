#include <metal_stdlib>
using namespace metal;

// maximum bounces per sample
#define MAX_BOUNCES 8

struct Camera {
    float3 origin;
    float3 lowerLeft;
    float3 horizontal;
    float3 vertical;
};

struct Material {
    float3 albedo;
    float3 emission;
    float  reflectivity;
    float  ior; // index of refraction, 1.0 for air, >1.0 for dielectric
};

struct SceneTriangle { float3 v0, v1, v2; uint matIndex; };
struct ScenePlane    { float3 normal; float  d;   uint matIndex; };
struct SceneSphere   { float3 center; float  radius; uint matIndex; };

inline float3 refractDir(float3 I, float3 N, float eta) {
    // I incoming, N normal, eta = η₁/η₂
    float cosI = dot(-I, N);
    float sin2T = eta*eta * (1.0 - cosI*cosI);
    if (sin2T > 1.0) return float3(0); // total internal reflection
    float cosT = sqrt(1.0 - sin2T);
    return eta * I + (eta * cosI - cosT) * N;
}

inline float fresnelSchlick(float cosTheta, float F0) {
    // Schlick’s approximation
    return F0 + (1.0 - F0)*pow(1.0 - cosTheta, 5.0);
}


inline float3 reflectDir(float3 I, float3 N) {
    return I - 2.0 * dot(I,N) * N;
}

struct Ray { float3 origin, dir; };

// RNG — LCG + [0,1)
inline uint lcg(thread uint &st) {
    st = st * 1664525u + 1013904223u;
    return st;
}
inline float rand01(thread uint &st) {
    return (float)(lcg(st) & 0x00FFFFFF) / float(0x01000000);
}

// cosine-weighted hemisphere
inline float3 randomHemisphere(float3 N, thread uint &st) {
    float u = rand01(st), v = rand01(st);
    float r = sqrt(u), theta = 2.0 * M_PI_F * v;
    // sample in tangent space
    float3 s = float3(r*cos(theta), r*sin(theta), sqrt(1.0 - u));
    // build basis
    float3 up = abs(N.z) < .9 ? float3(0,0,1) : float3(1,0,0);
    float3 tangent = normalize(cross(up, N));
    float3 bitan  = cross(N, tangent);
    return normalize(s.x*tangent + s.y*bitan + s.z*N);
}

// --------- Intersection --------------

// returns (t, normal, albedo) or t<0 if miss
inline float intersectTriangle(SceneTriangle tri, Ray r,
                               thread float3 &outN) {
    const float EPS = 1e-6;
    float3 e1 = tri.v1 - tri.v0, e2 = tri.v2 - tri.v0;
    float3 p = cross(r.dir, e2);
    float  det = dot(e1, p);
    if (fabs(det) < EPS) return -1.0;
    float inv = 1.0/det;
    float3 tvec = r.origin - tri.v0;
    float u = dot(tvec,p)*inv;
    if (u<0||u>1) return -1.0;
    float3 q = cross(tvec, e1);
    float v = dot(r.dir,q)*inv;
    if (v<0||u+v>1) return -1.0;
    float t = dot(e2,q)*inv;
    if (t<EPS) return -1.0;
    outN       = normalize(cross(e1,e2));
    return t;
    }

inline float intersectPlane(const ScenePlane pl, Ray r,
                            thread float3 &outN) {
    float denom = dot(pl.normal, r.dir);
    if (fabs(denom) < 1e-6) return -1.0;
    float t = -(dot(pl.normal, r.origin) + pl.d) / denom;
    if (t <= 0.0) return -1.0;
    outN   = pl.normal;
    return t;
}

inline float intersectSphere(const SceneSphere sp, Ray r,
                             thread float3 &outN) {
    float3 oc = r.origin - sp.center;
    float a = dot(r.dir, r.dir),
          b = dot(oc, r.dir),
          c = dot(oc, oc) - sp.radius*sp.radius;
    float disc = b*b - a*c;
    if (disc < 0.0) return -1.0;
    float t = (-b - sqrt(disc)) / a;
    if (t < 1e-6) return -1.0;
    float3 P = r.origin + t*r.dir;
    outN   = normalize(P - sp.center);
    return t;
}

// --------- Path‐trace kernel --------------

kernel void path_trace(
    texture2d<float, access::read_write> outTex   [[texture(0)]],
    device const SceneTriangle           *triangles [[buffer(1)]],
    constant uint                        &triCount  [[buffer(2)]],
    device const ScenePlane              *planes    [[buffer(3)]],
    constant uint                        &planeCount[[buffer(4)]],
    device const SceneSphere             *spheres   [[buffer(5)]],
    constant uint                        &sphereCount[[buffer(6)]],
    constant uint                        &frameIndex[[buffer(7)]],
    device const Material                *materials [[buffer(8)]],
    constant uint                        &matCount  [[buffer(9)]],
    constant Camera                      &cam        [[ buffer(10) ]],
    uint2                                gid       [[thread_position_in_grid]]
) {
    uint W = outTex.get_width(), H = outTex.get_height();
    if (gid.x>=W || gid.y>=H) return;

    // seed RNG per‐pixel+frame
    thread uint st = gid.x + gid.y*W + frameIndex*1973;

    // initialize ray & throughput
    float u = (float(gid.x) + rand01(st))/float(W);
    float v = 1.0 - (float(gid.y) + rand01(st))/float(H);
    Ray ray;
    ray.origin = cam.origin;
    ray.dir    = normalize(cam.lowerLeft + u*cam.horizontal + v*cam.vertical - cam.origin);
    // inside your path_trace, before the bounce loop:
    float3 throughput = float3(1.0);
    float3 L = float3(0.0);

    for (uint bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        // 1) Find the nearest intersection
        float  bestT   = 1e20;
        float3 bestN   = float3(0.0);
        uint   bestMat  = 0;

        // Triangles
        for (uint i = 0; i < triCount; ++i) {
            float3 nTmp;
            float  t = intersectTriangle(triangles[i], ray, nTmp);
            if (t > 0.0 && t < bestT) {
                bestT   = t;
                bestN   = nTmp;
                bestMat = triangles[i].matIndex;
            }
        }

        // Planes
        for (uint i = 0; i < planeCount; ++i) {
            float3 nTmp;
            float  t = intersectPlane(planes[i], ray, nTmp);
            if (t > 0.0 && t < bestT) {
                bestT   = t;
                bestN   = nTmp;
                bestMat = planes[i].matIndex;
            }
        }

        // Spheres
        for (uint i = 0; i < sphereCount; ++i) {
            float3 nTmp;
            float  t = intersectSphere(spheres[i], ray, nTmp);
            if (t > 0.0 && t < bestT) {
                bestT   = t;
                bestN   = nTmp;
                bestMat = spheres[i].matIndex;
            }
        }

        if (bestT > 1e19) {
            float  tt  = 0.5*(normalize(ray.dir).y + 1.0);
            float3 sky = mix(float3(0.2), float3(0.005, 0.007, 0.01), tt);
            L += throughput * sky;
            break;
        }

        // compute hit‐point
        float3 P = ray.origin + bestT * ray.dir;

        Material mat = materials[bestMat];

        L += throughput * mat.emission;

        // compute cosine of incidence
        float cosI = dot(ray.dir, bestN);
        bool entering = cosI < 0.0;
        float3 N = entering ? bestN : -bestN;

        // if this material has an ior > 1, treat it as dielectric:
        if (mat.ior > 1.0) {
            // decide indices
            float eta_i = entering ? 1.0 : mat.ior;
            float eta_t = entering ? mat.ior : 1.0;
            float eta   = eta_i / eta_t;

            // base reflectance at normal incidence
            float F0 = pow((eta_i - eta_t)/(eta_i + eta_t), 2.0);
            float R  = fresnelSchlick(fabs(cosI), F0);

            // sample reflect vs refract
            if (rand01(st) < R) {
                // reflect
                ray.origin = P + N * 0.001;
                ray.dir    = reflect(ray.dir, N);
                throughput *= R;       // attenuate by reflectance
            } else {
                // refract
                float3 T = refractDir(ray.dir, N, eta);
                ray.origin = P - N * 0.001; // move *into* the surface
                ray.dir    = T;
                throughput *= (1.0 - R);   // attenuate by transmittance
            }
            continue;
        }

        // otherwise fall back to your old metallic/diffuse mix:
        float p = rand01(st);
        if (p < mat.reflectivity) {
            // metallic reflect
            ray.origin = P + bestN * 0.001;
            ray.dir    = reflect(ray.dir, bestN);
            throughput *= mat.reflectivity;
        } else {
            // diffuse dome
            ray.origin = P + bestN * 0.001;
            ray.dir    = randomHemisphere(bestN, st);
            throughput *= mat.albedo * (1.0 - mat.reflectivity);
        }
    }

    // read & accumulate frame‐to‐frame
    float4 prev = (frameIndex>0) ? outTex.read(gid) : float4(0);
    float4 curr = float4(L,1.0);
    float4 accum= (prev*float(frameIndex) + curr)/float(frameIndex+1);

    outTex.write(accum, gid);
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
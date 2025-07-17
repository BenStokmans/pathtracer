#include <metal_stdlib>
#include "types.metal"
#include "rng.metal"
#include "bsdf.metal"
#include "intersection.metal"

using namespace metal;

// maximum bounces per sample
#define MAX_STACK_DEPTH 32
#define MAX_BOUNCES 20

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
    device const BVHNode                 *bvhNodes     [[buffer(11)]],
    constant uint                        &bvhNodeCount [[buffer(12)]],
    uint2                                gid       [[thread_position_in_grid]]
) {
    uint W = outTex.get_width(), H = outTex.get_height();
    if (gid.x>=W || gid.y>=H) return;

// seed RNG per‐pixel+frame
    thread uint st = gid.x + gid.y*W + frameIndex*1973;

    // generate a tiny random offset in [0,1) for AA
    float dx = rand01(st);
    float dy = rand01(st);

    // initialize primary ray with jittered uv inside pixel
    float u = (float(gid.x) + dx) / float(W);
    float v = 1.0 - (float(gid.y) + dy) / float(H);

    Ray ray;
    ray.origin = cam.origin;
    ray.dir    = normalize(cam.lowerLeft + u*cam.horizontal + v*cam.vertical - cam.origin);
    float3 throughput = float3(1.0);
    float3 L = float3(0.0);

    for (uint bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        // 1) Find the nearest intersection
        float  bestT   = 1e20;
        float3 bestN   = float3(0.0);
        uint   bestMat  = 0;

        int stack[MAX_STACK_DEPTH];
        int  sp = 0;
        stack[sp++] = 0; // root node

        while (sp > 0) {
            int ni = stack[--sp];
            BVHNode node = bvhNodes[ni];
            if (!intersectAABB(node.bboxMin, node.bboxMax, ray)) continue;
            if (node.count > 0) {
                int start = node.leftFirst;
                for (uint i=0; i<node.count; ++i) {
                    float3 nTmp;
                    float  t = intersectTriangle(triangles[start+i], ray, nTmp);
                    if (t > 0.0 && t < bestT) {
                        bestT   = t;
                        bestN   = nTmp;
                        bestMat = triangles[start+i].matIndex;
                    }
                }
            } else {
                int left  = node.leftFirst;
                int right = node.rightFirst;
                if (sp + 2 <= MAX_STACK_DEPTH) {
                    stack[sp++] = left;
                    stack[sp++] = right;
                }
            }
        }

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

        // Russian roulette termination after 4 bounces
        if (bounce >= 4) {
            // probability of survival = max RGB throughput, clamped to [0.05,1]
            float p_rr = max(max(throughput.x, throughput.y), throughput.z);
            p_rr = clamp(p_rr, 0.05, 1.0);
            if (rand01(st) > p_rr) {
                break;
            }
            throughput /= p_rr;
        }

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

            if (rand01(st) < R) {
                // reflect
                ray.origin = P + N*0.001;
                ray.dir    = reflectDir(ray.dir,N);
            } else {
                // refract
                ray.origin = P - N*0.001;
                ray.dir    = refractDir(ray.dir,N,eta);
            }
            continue;
        }

        float p_spec = mat.reflectivity;
        float p_diff = 1.0 - p_spec;
        float u_b    = rand01(st);

        if (u_b < p_spec) {
            ray.origin  = P + bestN * 0.001;
            ray.dir     = reflectDir(ray.dir, bestN);
            throughput *= (1.0 / p_spec);
        } else {
            ray.origin  = P + bestN * 0.001;
            ray.dir     = randomHemisphere(bestN, st);
            throughput *= mat.albedo / p_diff;
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
    float3 ldr = sqrt(hdr);
    return float4(ldr, 1);
}
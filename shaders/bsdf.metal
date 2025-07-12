#include <metal_stdlib>
using namespace metal;

// reflect
inline float3 reflectDir(float3 I, float3 N) {
    return I - 2.0 * dot(I, N) * N;
}

// refract (η = η₁/η₂)
inline float3 refractDir(float3 I, float3 N, float eta) {
    float cosI  = dot(-I, N);
    float sin2T = eta*eta * (1.0 - cosI*cosI);
    if (sin2T > 1.0) return float3(0);       // TIR
    float cosT = sqrt(1.0 - sin2T);
    return eta * I + (eta * cosI - cosT) * N;
}

// Schlick’s Fresnel
inline float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(1 - cosTheta, 5);
}

// cosine-weighted hemisphere
inline float3 randomHemisphere(float3 N, thread uint &st) {
    float u = rand01(st), v = rand01(st);
    float r     = sqrt(u),
          theta = 2.0f * M_PI_F * v;
    float3 s = float3(r*cos(theta), r*sin(theta), sqrt(1 - u));
    float3 up      = abs(N.z) < .9 ? float3(0,0,1) : float3(1,0,0);
    float3 tangent = normalize(cross(up, N));
    float3 bitan   = cross(N, tangent);
    return normalize(s.x*tangent + s.y*bitan + s.z*N);
}
#ifndef SCENEPRIMITIVES_H
#define SCENEPRIMITIVES_H

#pragma once

struct Triangle {
    simd::float3 v0;
    simd::float3 v1;
    simd::float3 v2;
    uint32_t matIndex;
};

struct Plane {
    simd::float3 normal;
    float d;
    uint32_t matIndex;
};

struct Sphere {
    simd::float3 center;
    float radius;
    uint32_t matIndex;
};

#endif //SCENEPRIMITIVES_H

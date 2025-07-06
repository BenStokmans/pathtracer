#ifndef SCENEPRIMITIVES_H
#define SCENEPRIMITIVES_H

#pragma once
#include <simd/simd.h>

struct SceneTriangle {
    simd::float3 v0;
    simd::float3 v1;
    simd::float3 v2;
    simd::float3 albedo;
    float        reflectivity;
};

struct ScenePlane {
    simd::float3 normal;
    float        d;
    simd::float3 albedo;
    float        reflectivity;
};

struct SceneSphere {
    simd::float3 center;
    float        radius;
    simd::float3 albedo;
    float        reflectivity;
};

#endif //SCENEPRIMITIVES_H

#ifndef MATERIAL_H
#define MATERIAL_H

struct Material {
    simd::float3 albedo; // diffuse color
    simd::float3 emission; // emissive color (light)
    float reflectivity; // [0..1]  0 = pure diffuse, 1 = perfect mirror
    float ior; // >1 means dielectric
};

#endif //MATERIAL_H

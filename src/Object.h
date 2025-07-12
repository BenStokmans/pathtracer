#ifndef OBJECT_H
#define OBJECT_H
#include <vector>
#include <simd/matrix_types.h>

#include "Primitives/Primitives.h"

struct Object {
    uint32_t firstTriangle{}; // index into your big triangle array
    uint32_t triCount{};
    uint32_t materialIndex{}; // or per‐triangle if you want
    simd::float4x4 transform;
};

// TODO: define these in a logical spot
inline std::vector<Object> objects;
inline std::vector<Triangle> triangles;

#endif //OBJECT_H

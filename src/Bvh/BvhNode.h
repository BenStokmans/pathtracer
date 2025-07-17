#ifndef BVHNODE_H
#define BVHNODE_H
#include <cstdint>
#include <simd/vector_types.h>

struct BVHNode {
    simd::float3 bboxMin;
    simd::float3 bboxMax;
    uint32_t leftFirst; // leaf: first tri; inner: left child index
    uint32_t rightFirst; // inner: right child index
    uint32_t count; // leaf: tri count;   inner: 0
};

#endif //BVHNODE_H

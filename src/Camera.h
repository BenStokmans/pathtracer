#ifndef CAMERA_H
#define CAMERA_H
#include <simd/vector_types.h>

struct Camera {
    simd::float3 origin;
    simd::float3 lowerLeft;
    simd::float3 horizontal;
    simd::float3 vertical;
};


#endif //CAMERA_H

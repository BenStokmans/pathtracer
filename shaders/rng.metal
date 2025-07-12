#include <metal_stdlib>
using namespace metal;

// simple LCG → [0,1)
inline uint lcg(thread uint &st) {
    st = st * 1664525u + 1013904223u;
    return st;
}
inline float rand01(thread uint &st) {
    return (float)(lcg(st) & 0x00FFFFFF) / float(0x01000000);
}

#ifndef BVHBUILDER_H
#define BVHBUILDER_H
#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>
#include <simd/common.h>

#include "BvhNode.h"
#include "../Primitives/Primitives.h"

static constexpr int kMaxBVHNodes = 1000000; // tune to your GPU budget
static constexpr size_t kMaxTriangles = 500000; // likewise

struct BvhBuilder {
    static int buildBVH(
        int start,
        int end,
        const std::vector<Triangle> &tris,
        std::vector<BVHNode> &nodes,
        std::vector<int> &triIndices
    ) {
        // Optional: on the very first call, reserve enough space so no reallocation ever happens.
        // (You could also do this once externally, e.g. in setupScene().)
        if (start == 0 && end == (int) tris.size()) {
            nodes.reserve(tris.size() * 2);
        }

        // 1) Allocate a new node slot
        int nodeIndex = static_cast<int>(nodes.size());
        nodes.emplace_back(); // may reallocate, but we won't keep a reference

        // 2) Compute bounding box over triangles [start,end)
        simd::float3 bbMin = {HUGE_VALF, HUGE_VALF, HUGE_VALF};
        simd::float3 bbMax = {-HUGE_VALF, -HUGE_VALF, -HUGE_VALF};
        for (int i = start; i < end; ++i) {
            const Triangle &T = tris[triIndices[i]];
            bbMin = simd::min(bbMin, T.v0);
            bbMin = simd::min(bbMin, T.v1);
            bbMin = simd::min(bbMin, T.v2);
            bbMax = simd::max(bbMax, T.v0);
            bbMax = simd::max(bbMax, T.v1);
            bbMax = simd::max(bbMax, T.v2);
        }

        // 3) Write the bbox into the freshly‐allocated node
        nodes[nodeIndex].bboxMin = bbMin;
        nodes[nodeIndex].bboxMax = bbMax;

        int count = end - start;
        constexpr int leafThresh = 4;

        if (count <= leafThresh) {
            // 3a) Leaf
            nodes[nodeIndex].leftFirst = start; // index into triIndices
            nodes[nodeIndex].count = count; // number of tris
            nodes[nodeIndex].rightFirst = 0; // unused
        } else {
            // 3b) Inner node: split
            simd::float3 extent = bbMax - bbMin;
            int axis = (extent.x > extent.y
                            ? (extent.x > extent.z ? 0 : 2)
                            : (extent.y > extent.z ? 1 : 2));
            int mid = (start + end) / 2;
            std::nth_element(
                triIndices.begin() + start,
                triIndices.begin() + mid,
                triIndices.begin() + end,
                [&](int a, int b) {
                    float ca = ((tris[a].v0 + tris[a].v1 + tris[a].v2) / 3.0f)[axis];
                    float cb = ((tris[b].v0 + tris[b].v1 + tris[b].v2) / 3.0f)[axis];
                    return ca < cb;
                }
            );

            // Recurse
            int leftChild = buildBVH(start, mid, tris, nodes, triIndices);
            int rightChild = buildBVH(mid, end, tris, nodes, triIndices);

            // Fill inner‐node fields
            nodes[nodeIndex].leftFirst = leftChild;
            nodes[nodeIndex].rightFirst = rightChild;
            nodes[nodeIndex].count = 0; // marker for “inner”
        }

        return nodeIndex;
    }

    static void exportBVHWireframeOBJ(const std::vector<BVHNode> &nodes, const std::string &path) {
        std::ofstream out(path);
        if (!out) return;

        int vertexOffset = 1;
        // for each node, emit its 8 corners and the 12 edges as lines
        for (auto &n: nodes) {
            auto &mn = n.bboxMin;
            auto &mx = n.bboxMax;
            // the 8 corners of the box
            std::array<simd::float3, 8> C = {
                {
                    {mn.x, mn.y, mn.z},
                    {mx.x, mn.y, mn.z},
                    {mx.x, mx.y, mn.z},
                    {mn.x, mx.y, mn.z},
                    {mn.x, mn.y, mx.z},
                    {mx.x, mn.y, mx.z},
                    {mx.x, mx.y, mx.z},
                    {mn.x, mx.y, mx.z},
                }
            };
            // emit as OBJ vertices
            for (auto &v: C) {
                out << "v " << v.x << " " << v.y << " " << v.z << "\n";
            }
            // the 12 edges
            static const int E[12][2] = {
                {0, 1}, {1, 2}, {2, 3}, {3, 0},
                {4, 5}, {5, 6}, {6, 7}, {7, 4},
                {0, 4}, {1, 5}, {2, 6}, {3, 7}
            };
            // emit them as OBJ “l”ine elements
            for (auto &e: E) {
                out << "l "
                        << (vertexOffset + e[0]) << " "
                        << (vertexOffset + e[1]) << "\n";
            }
            vertexOffset += 8;
        }
        out.close();
    }
};


#endif //BVHBUILDER_H

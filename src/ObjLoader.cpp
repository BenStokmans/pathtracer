#include "ObjLoader.h"

#include <simd/simd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "Object.h"
#include "Primitives/Primitives.h"

bool ObjLoader::loadObj(const std::string &filename, uint32_t materialIndex) {
    std::ifstream in{filename};
    if (!in) {
        std::cerr << "Failed to open OBJ: " << filename << "\n";
        return false;
    }

    // temporary storage for vertex positions
    std::vector<simd::float3> positions;
    positions.reserve(1024);

    uint32_t startIdx = static_cast<uint32_t>(triangles.size());
    uint32_t triCount = 0;

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss{line};
        std::string tag;
        iss >> tag;
        if (tag == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(simd::float3{x, y, z});
        } else if (tag == "f") {
            // supports: f v1 v2 v3  or f v1/... v2/... v3/...
            auto parseIdx = [&](const std::string &token) {
                const size_t slash = token.find('/');
                return static_cast<uint32_t>(std::stoul(
                           slash == std::string::npos ? token : token.substr(0, slash)
                       )) - 1;
            };
            std::string a, b, c;
            iss >> a >> b >> c;
            uint32_t i0 = parseIdx(a),
                    i1 = parseIdx(b),
                    i2 = parseIdx(c);

            Triangle T;
            T.v0 = positions[i0];
            T.v1 = positions[i1];
            T.v2 = positions[i2];
            T.matIndex = materialIndex;

            // compute un‐normalized face‐normal
            simd::float3 e1 = T.v1 - T.v0;
            simd::float3 e2 = T.v2 - T.v0;

            // if faceNormal ⋅ some reference direction is >0, winding is wrong
            // e.g. if you assume y‐up and want outward normals dot +Y to be positive:
            if (simd::float3 faceNormal = simd::cross(e1, e2); simd::dot(faceNormal, simd::float3{0, 1, 0}) < 0) {
                // swap v1<->v2 to flip winding
                std::swap(T.v1, T.v2);
            }
            triangles.push_back(T);
            ++triCount;
        }
    }

    // record the new object
    Object obj;
    obj.firstTriangle = startIdx;
    obj.triCount = triCount;
    obj.materialIndex = materialIndex;
    obj.transform = matrix_identity_float4x4;
    objects.push_back(obj);

    std::cout << "Loaded OBJ: " << filename
            << " (triangles: " << triCount << ")\n";
    return true;
}

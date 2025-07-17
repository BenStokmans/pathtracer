#include "ObjLoader.h"

#include <simd/simd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
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
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss{line};

        std::string tag;
        iss >> tag;
        if (tag == "v") {
            // vertex position
            float x, y, z;
            iss >> x >> y >> z;
            simd::float3 pos = {x, y, z};
            positions.push_back(pos);
        } else if (tag == "f") {
            // parse *all* indices in this face
            std::vector<int> idxList;
            std::string tok;
            while (iss >> tok) {
                // strip off anything after the first '/'
                size_t slash = tok.find('/');
                std::string vStr = (slash == std::string::npos ? tok : tok.substr(0, slash));

                // OBJ allows negative (relative) indices
                int i = std::stoi(vStr);
                if (i < 0) {
                    i = int(positions.size()) + i; // relative to end
                } else {
                    i = i - 1; // 1-based → 0-based
                }
                idxList.push_back(i);
            }

            // need at least 3 verts to form triangles
            if (idxList.size() < 3) continue;

            // fan-triangulate: (0,i,i+1)
            for (size_t j = 1; j + 1 < idxList.size(); ++j) {
                Triangle T;
                T.v0 = positions[idxList[0]];
                T.v1 = positions[idxList[j]];
                T.v2 = positions[idxList[j + 1]];
                T.matIndex = materialIndex;

                // you can compute a flat normal here if you want:
                // simd::float3 e1 = T.v1 - T.v0;
                // simd::float3 e2 = T.v2 - T.v0;
                // T.normal = simd::normalize(simd::cross(e1, e2));

                triangles.push_back(T);
                ++triCount;
            }
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

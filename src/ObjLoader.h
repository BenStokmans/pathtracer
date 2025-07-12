#ifndef OBJLOADER_H
#define OBJLOADER_H

#pragma once
#include <string>

class ObjLoader {
public:
    // Simple Wavefront OBJ loader: parses positions and triangular faces only.
    // Appends parsed Triangles into the global `triangles` vector,
    // and records an Object entry in `objects`.
    static bool loadObj(const std::string &filename, uint32_t materialIndex);
};


#endif //OBJLOADER_H

struct Camera {
    float3 origin;
    float3 lowerLeft;
    float3 horizontal;
    float3 vertical;
};

struct Material {
    float3 albedo;
    float3 emission;
    float  reflectivity;
    float  ior; // index of refraction, 1.0 for air, >1.0 for dielectric
};

struct SceneTriangle { float3 v0, v1, v2; uint matIndex; };
struct ScenePlane    { float3 normal; float  d;   uint matIndex; };
struct SceneSphere   { float3 center; float  radius; uint matIndex; };

struct Ray { float3 origin, dir; };


// returns (t, normal, albedo) or t<0 if miss
inline float intersectTriangle(SceneTriangle tri, Ray r,
                               thread float3 &outN) {
    const float EPS = 1e-6;
    float3 e1 = tri.v1 - tri.v0, e2 = tri.v2 - tri.v0;
    float3 p = cross(r.dir, e2);
    float  det = dot(e1, p);
    if (fabs(det) < EPS) return -1.0;
    float inv = 1.0/det;
    float3 tvec = r.origin - tri.v0;
    float u = dot(tvec,p)*inv;
    if (u<0||u>1) return -1.0;
    float3 q = cross(tvec, e1);
    float v = dot(r.dir,q)*inv;
    if (v<0||u+v>1) return -1.0;
    float t = dot(e2,q)*inv;
    if (t<EPS) return -1.0;
    outN       = normalize(cross(e1,e2));
    return t;
    }

inline float intersectPlane(const ScenePlane pl, Ray r,
                            thread float3 &outN) {
    float denom = dot(pl.normal, r.dir);
    if (fabs(denom) < 1e-6) return -1.0;
    float t = -(dot(pl.normal, r.origin) + pl.d) / denom;
    if (t <= 0.0) return -1.0;
    outN   = pl.normal;
    return t;
}

inline float intersectSphere(const SceneSphere sp, Ray r,
                             thread float3 &outN) {
    float3 oc = r.origin - sp.center;
    float a = dot(r.dir, r.dir),
          b = dot(oc, r.dir),
          c = dot(oc, oc) - sp.radius*sp.radius;
    float disc = b*b - a*c;
    if (disc < 0.0) return -1.0;
    float t = (-b - sqrt(disc)) / a;
    if (t < 1e-6) return -1.0;
    float3 P = r.origin + t*r.dir;
    outN   = normalize(P - sp.center);
    return t;
}

inline bool intersectAABB(float3 mn, float3 mx, Ray r) {
    float3 inv = 1.0 / r.dir;
    float3 t0  = (mn - r.origin) * inv;
    float3 t1  = (mx - r.origin) * inv;
    float3 tmin = min(t0, t1), tmax = max(t0, t1);
    float tnear = max(max(tmin.x, tmin.y), tmin.z);
    float tfar  = min(min(tmax.x, tmax.y), tmax.z);
    return tfar >= max(tnear, 0.0);
}
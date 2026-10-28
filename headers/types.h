#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "materials.h"

typedef struct { float x,y,z; } Vector3f;

static inline Vector3f vec3(float x, float y, float z) {
    Vector3f v = {x,y,z}; return v;
}
static inline Vector3f vadd(Vector3f a, Vector3f b) {
    return vec3(a.x+b.x, a.y+b.y, a.z+b.z);
}
static inline Vector3f vsub(Vector3f a, Vector3f b) {
    return vec3(a.x-b.x, a.y-b.y, a.z-b.z);
}
static inline Vector3f vscale(Vector3f a, float s) {
    return vec3(a.x*s, a.y*s, a.z*s);
}
static inline float vdot(Vector3f a, Vector3f b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static inline Vector3f vcross(Vector3f a, Vector3f b) {
    return vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}
static inline float vlen2(Vector3f v) {
    return vdot(v,v);
}
static inline Vector3f vnorm(Vector3f v) {
    float inv = 1.0f / sqrtf(vlen2(v));
    return vscale(v, inv);
}

typedef struct {
    Vector3f position;
    const Spectrum *color;
    float intensity;
} Light;

typedef struct {
    Vector3f position;
    float radius;
    float radius2;
    const Material *material;
    bool isLightSource;
} Sphere;

static inline Light newLight(Vector3f pos, const Spectrum *col, float intensity) {
    Light l = {pos, col, intensity};
    return l;
}

static inline Sphere newSphere(Vector3f pos, float r, const Material *mat) {
    Sphere s = {pos, r, r*r, mat};
    return s;
}

typedef struct {
    bool hit;   // if the ray intersects the sphere
    float t;    // parameter for which the ray intersects the sphere
} Hit;

static inline Hit intersectSphere(const Sphere *s, Vector3f ro, Vector3f rd) {
    Hit hit = {false, 0.0f};
    Vector3f L = vsub(s->position, ro);   // vector from the ray origin to the sphere's center
    float tca = vdot(L, rd);                // to camera "angle"
    
    if (tca < 0) {
        return hit;
    }
    
    float d2 = vlen2(L) - tca * tca;        // squared distance from the ray to the sphere's center
    
    if (d2 > s->radius2) {
        return hit;
    }
    
    float thc = sqrtf(s -> radius2 - d2);
    float t0 = tca - thc;
    
    if (t0 < 0) {
        t0 = tca + thc;
    }
    
    if (t0 > 0) {
        hit.hit = true;
        hit.t = t0;
    }
    
    return hit;
}

typedef struct {
    float intensity;
    bool deterministic;
} TraceResult;

typedef struct {
    float accumR, accumG, accumB;   // Linear XYZ sum
    float lastR,  lastG,  lastB;    // Previous average
    float coneFractionDiffuse, coneFractionSpecular;     // Cached coverage fractions
    uint32_t samples;
    uint8_t stableCount;
    bool converged;
} PixelData;

#endif

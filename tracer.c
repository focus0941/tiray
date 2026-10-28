#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "materials.h"
#include "types.h"
#include "platform.h"       // includes <os.h>, <libndls.h>, and SCR_W, SCR_H

#define MAX_RAY_DEPTH            10
#define NUM_SPHERES              6
#define NUM_LIGHTS               1
#define MIN_WAVELENGTH           400.0f
#define MAX_WAVELENGTH           750.0f
#define NUM_WAVELENGTHS          8
#define WAVELENGTH_WEIGHT        ((MAX_WAVELENGTH - MIN_WAVELENGTH) / (float)NUM_WAVELENGTHS)
#define DISPLAY_EXPOSURE         1.0f
#define DELTA_THRESHOLD          0.002f   // 0.2% relative change per frame
#define CONVERGE_FRAMES          10
#define MAX_SAMPLES_PER_PIXEL    1000
#define NUM_GAMMA_VALUES         4096

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923f
#endif

static PixelData pixels[SCR_W * SCR_H];

static float invYIntegral;
static float xyz[NUM_WAVELENGTHS][3];
static float wavelengths[NUM_WAVELENGTHS];
static uint8_t gammaLUT[NUM_GAMMA_VALUES];
static Vector3f rayDir[SCR_W * SCR_H];
static const float epsilon = 1e-4f;
static uint32_t *activePixels = NULL;
static size_t numActive = 0;
static uint16_t *screenBuffer = NULL; 

static float skyXYZ[3];
static const Spectrum skySpectrum = DARK_BLUE_SPECTRUM;

static const Material MATTE_WHITE_MAT = {LIGHT_GRAY_SPECTRUM, ZERO_SPECTRUM, ZERO_SPECTRUM, ZERO_SPECTRUM, 1.0f};
static const Material SPECULAR_RED_MAT = {ZERO_SPECTRUM, RED_SPECTRUM, ZERO_SPECTRUM, ZERO_SPECTRUM, 0.0f};
static const Material MATTE_GREEN_MAT = {DARK_GREEN_SPECTRUM, ZERO_SPECTRUM, ZERO_SPECTRUM, ZERO_SPECTRUM, 1.0f};
static const Material SPECULAR_BLUE_MAT = {ZERO_SPECTRUM, DARK_BLUE_SPECTRUM, ZERO_SPECTRUM, ZERO_SPECTRUM, 0.0f};
static const Material MATTE_GLASS_MAT = {ZERO_SPECTRUM, FULL_SPECTRUM, FULL_SPECTRUM, GLASS_IOR, 0.0f};
static const Material GOLD_MAT = {ZERO_SPECTRUM, YELLOW_SPECTRUM, ZERO_SPECTRUM, ZERO_SPECTRUM, 0.0f};
static Material red_mat;
static Material blue_mat;

static Sphere spheres[NUM_SPHERES];
static Light lights[NUM_LIGHTS];

static Spectrum makeGaussian(float center, float width, float amplitude) {
    Spectrum s;
    
    for (int i = 0; i < NUM_WAVELENGTHS; i++) {
        float w = MIN_WAVELENGTH + ((MAX_WAVELENGTH - MIN_WAVELENGTH) * i) / (NUM_WAVELENGTHS - 1);
        float t = (w - center) / width;
        
        s.spectrum[i] = amplitude * expf(-0.5f * t * t);
        
        // clamp to 1.0 to stay physically correct
        if (s.spectrum[i] > 1.0f) {
            s.spectrum[i] = 1.0f;
        }
    }
    
    return s;
}

static inline uint32_t randInt(uint32_t idx) {
    uint32_t h = idx * 0x3504f333u;
    h ^= h >> 16;
    h *= 0xf1bbcdcbu;
    h ^= h >> 13;
    
    return h;
}

static inline float randFloat(uint32_t idx, int seed) {
    uint32_t r = randInt(idx + seed);
    
    return (r & 0xffffff) / 16777216.0f;
}

void initTables(void) {
    for (int i = 0; i < NUM_WAVELENGTHS; i++) {
        wavelengths[i] = MIN_WAVELENGTH + ((MAX_WAVELENGTH - MIN_WAVELENGTH) * i) / (NUM_WAVELENGTHS - 1);
    }

    for (int i = 0; i < NUM_WAVELENGTHS; i++) {
        float w = wavelengths[i];

        float t1 = (w - 442.0f) * ((w < 442.0f) ? 0.0624f : 0.0374f);
        float t2 = (w - 599.8f) * ((w < 599.8f) ? 0.0264f : 0.0323f);
        float t3 = (w - 501.1f) * ((w < 501.1f) ? 0.0490f : 0.0382f);

        float X = 0.362f * expf(-0.5f * t1*t1) + 1.056f * expf(-0.5f * t2*t2) - 0.065f * expf(-0.5f  * t3*t3);

        t1 = (w - 568.8f) * ((w < 568.8f) ? 0.0213f : 0.0247f);
        t2 = (w - 530.9f) * ((w < 530.9f) ? 0.0613f : 0.0322f);

        float Y = 0.821f * expf(-0.5f * t1*t1) + 0.286f * expf(-0.5f * t2*t2);

        t1 = (w - 437.0f) * ((w < 437.0f) ? 0.0845f : 0.0278f);
        t2 = (w - 459.0f) * ((w < 459.0f) ? 0.0385f : 0.0725f);

        float Z = 1.217f * expf(-0.5f * t1*t1) + 0.681f * expf(-0.5f * t2*t2);

        xyz[i][0] = X;
        xyz[i][1] = Y;
        xyz[i][2] = Z;
    }
    
    // normalization of exposure
    float yInt = 0.0f;
    
    for (int i = 0; i < NUM_WAVELENGTHS; i++) {
        yInt += xyz[i][1] * WAVELENGTH_WEIGHT;
    }
    
    invYIntegral = (yInt > 0.0f) ? 1.0f / yInt : 1.0f;
}

static void initGammaLUT(void) {
    for (int i = 0; i < NUM_GAMMA_VALUES; i++) {
        float x = (float)i / (NUM_GAMMA_VALUES - 1.0f);
        float y;
        
        if (x <= 0.0031308f) {
            y = 12.92f * x;
        } else {
            y = 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
        }
        
        gammaLUT[i] = (uint8_t)(y * 255.0f + 0.5f);
    }
}

static void initSkyRGB(void) {
    float X = 0;
    float Y = 0;
    float Z = 0;

    for (int wl = 0; wl < NUM_WAVELENGTHS; wl++) {
        float s = skySpectrum.spectrum[wl];   // fixed
        X += s * xyz[wl][0] * WAVELENGTH_WEIGHT;
        Y += s * xyz[wl][1] * WAVELENGTH_WEIGHT;
        Z += s * xyz[wl][2] * WAVELENGTH_WEIGHT;
    }

    skyXYZ[0] = X;
    skyXYZ[1] = Y;
    skyXYZ[2] = Z;
}

static void initScene(void) {
    red_mat = newMaterial((Spectrum)ZERO_SPECTRUM, makeGaussian(600, 40, 1.0f), (Spectrum)ZERO_SPECTRUM, (Spectrum)ZERO_SPECTRUM, 0.0f);
    blue_mat = newMaterial((Spectrum)ZERO_SPECTRUM, (Spectrum)DARK_BLUE_SPECTRUM/*makeGaussian(450, 80, 1.0f)*/, (Spectrum)ZERO_SPECTRUM, (Spectrum)ZERO_SPECTRUM, 0.2f); // Pure blue
    
    spheres[0] = newSphere(vec3( 0.0, -1000004,  -20.0), 1000000, &MATTE_WHITE_MAT);
    spheres[1] = newSphere(vec3(-5.5,      1.0,  -13.0),       4, &SPECULAR_RED_MAT);
    spheres[2] = newSphere(vec3( 0.5,      0.5,  -15.0),       3, &MATTE_GREEN_MAT);
    spheres[3] = newSphere(vec3( 4.5,     -1.5,  -14.0),       2, &blue_mat);
    spheres[4] = newSphere(vec3( 0.9,     -1.5, -10.25),     1.6, &MATTE_GLASS_MAT);
    spheres[5] = newSphere(vec3( 2.8,    -1.35,   -6.0),     0.4, &GOLD_MAT);

    lights[0] = newLight(vec3(0, 200, 0), &FULL_SPECTRUM_VAR, 1.0e5f);
}

static void initRays(void) {
    const float fov = 0.577f;         // actually tan(fov/2)
    const float aspect = (float)SCR_W / SCR_H;
    const float invW = 1.0f / SCR_W;
    const float invH = 1.0f / SCR_H;

    for (int y = 0; y < SCR_H; y++) {
        float py = (1.0f - 2.0f * (y + 0.5f) * invH) * fov;

        for (int x = 0; x < SCR_W; x++) {
            float px = (2.0f * (x + 0.5f) * invW - 1.0f) * aspect * fov;
            rayDir[y * SCR_W + x] = vnorm(vec3(px, py, -1.0f));
        }
    }
}

static bool inShadow(Vector3f origin, Vector3f dir, float maxDist2, const Sphere *ignore) {
    for (int i = 0; i < NUM_SPHERES; i++) {
        const Sphere *s = &spheres[i];
        
        if (s == ignore) {
            continue;
        }

        Hit is = intersectSphere(s, origin, dir);
        
        if (is.hit && is.t > epsilon && is.t * is.t < maxDist2) {
            return true;
        }
    }
    
    return false;
}

// Fraction of the cone (apex at hitPos, axis dir, half-angle theta) covered by a sphere at spherePos with radius R.
// Returns 0-1. For multiple spheres, sum the fractions (counts overlaps !)
static float findConeCoverage(Vector3f hitPos, Vector3f dir, float theta, Vector3f spherePos, float R) {
    Vector3f L = vsub(spherePos, hitPos);
    float dist2 = vlen2(L);

    if (dist2 <= R * R) {
        return 1.0f;        // hitPos inside sphere
    }

    float dist  = sqrtf(dist2);
    float alpha = asinf(R / dist);                // sphere angular radius
    Vector3f w  = vscale(L, 1.0f / dist);

    float cosDelta = vdot(dir, w);
    
    if (cosDelta > 1.0f) {
        cosDelta =  1.0f;
    }
    
    if (cosDelta < -1.0f) {
        cosDelta = -1.0f;
    }
    
    float delta = acosf(cosDelta);

    // Disjoint ranges
    if (delta >= theta + alpha) {       // no overlap
        return 0.0f;
    }
    if (delta + theta <= alpha) {       // cone inside sphere cap
        return 1.0f;
    }
    
    // Sphere cap inside cone
    if (delta + alpha <= theta) {
        float cA = cosf(alpha);
        float cT = cosf(theta);
        float denom = 1.0f - cT;
        
        if (denom < epsilon) {
            return 0.0f;
        }
        
        return (1.0f - cA) / denom;
    }

    // Partial overlap
    float rc = 2.0f * sinf(theta * 0.5f);
    float rs = 2.0f * sinf(alpha * 0.5f);
    float dc = 2.0f * sinf(delta * 0.5f);

    if (dc >= rc + rs) {
        return 0.0f;
    }
    if (dc + rc <= rs) {
        return 1.0f;
    }
    if (dc + rs <= rc) {
        return (rs * rs) / (rc * rc);
    }

    float d1 = (dc*dc + rc*rc - rs*rs) / (2.0f * dc * rc);
    float d2 = (dc*dc + rs*rs - rc*rc) / (2.0f * dc * rs);
    
    if (d1 > 1.0f) {
        d1 = 1.0f;
    }
    if (d1 < -1.0f) {
        d1 = -1.0f;
    }
    if (d2 > 1.0f) {
        d2 = 1.0f;
    }
    if (d2 < -1.0f) {
        d2 = -1.0f;
    }

    float a1   = rc*rc * acosf(d1);
    float a2   = rs*rs * acosf(d2);
    float a3   = 0.5f * sqrtf((-dc+rc+rs) * (dc+rc-rs) * (dc-rc+rs) * (dc+rc+rs));
    float lens = a1 + a2 - a3;
    float frac = lens / (M_PI * rc * rc);

    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    
    return frac;
}

// Cosine-weighted solid-angle fraction of a sphere seen from a surface point with the given normal. Returns 0-1 but (counts overlaps !)
static float cosineCoverage(Vector3f hitPos, Vector3f normal, Vector3f spherePos, float R) {
    Vector3f L = vsub(spherePos, hitPos);
    float dist2 = vlen2(L);
    
    if (dist2 <= R * R) {
        return 1.0f;          // inside sphere
    }

    float dist = sqrtf(dist2);
    float sinA = R / dist;
    
    if (sinA >= 1.0f) {
        return 1.0f;            // degenerate (shouldn't happen)
    }
    
    float sin2A = sinA * sinA;
    Vector3f w = vscale(L, 1.0f / dist);
    float cosDelta = vdot(normal, w);
    
    if (cosDelta <= 0.0f) {
        return 0.0f;        // sphere below local horizon
    }
    
    return cosDelta * sin2A;
}

static inline Vector3f roughenDirection(Vector3f idealDir, Vector3f normal, float roughness, int idx, int seedOffset) {
    if (roughness <= 0.0f) {
        return idealDir;
    }

    float theta = roughness * M_PI_2;
    float cosTheta = cosf(theta);

    // Uniform in solid angle: cosA linear in [cosTheta, 1]
    float rand1 = randFloat(idx, seedOffset);
    float cosA = 1.0f - rand1 * (1.0f - cosTheta);
    float sinA = sqrtf(1.0f - cosA * cosA);
    float phi = 2.0f * M_PI * randFloat(idx, seedOffset + 1);

    Vector3f axis = (fabsf(idealDir.x) > 0.9f) ? vec3(0,1,0) : vec3(1,0,0);
    Vector3f basisU = vnorm(vcross(axis, idealDir));
    Vector3f basisV = vcross(idealDir, basisU);
    Vector3f pert = vadd(vscale(basisU, cosf(phi) * sinA), vscale(basisV, sinf(phi) * sinA));
    
    return vnorm(vadd(vscale(idealDir, cosA), pert));
}

static inline Vector3f reflectDir(Vector3f rd, Vector3f n) {
    return vnorm(vsub(rd, vscale(n, 2.0f * vdot(rd, n))));
}

static inline float fresnelDielectric(float cosi, float n1, float n2) {
    float R0 = (n1 - n2) / (n1 + n2);
    R0 *= R0;
    
    return R0 + (1.0f - R0) * powf(1.0f - cosi, 5.0f);
}

static inline Vector3f refractDir(Vector3f rd, Vector3f n, float eta, float cosi, float k) {
    return vnorm(vadd(vscale(rd, eta), vscale(n, eta * cosi - sqrtf(k))));
}

static const Sphere *closestHit(Vector3f ro, Vector3f rd, float *tOut) {
    float nearest = INFINITY;
    const Sphere *best = NULL;
    
    for (int i = 0; i < NUM_SPHERES; i++) {
        Hit is = intersectSphere(&spheres[i], ro, rd);
        
        if (is.hit && is.t < nearest) {
            nearest = is.t;
            best = &spheres[i];
        }
    }
    
    *tOut = nearest;
    
    return best;
}

static inline Vector3f surfaceNormal(Vector3f hitPos, const Sphere *hit, Vector3f rd, bool *inside) {
    Vector3f n = vnorm(vsub(hitPos, hit->position));
    
    if (vdot(rd, n) > 0.0f) {
        *inside = true;
        
        return vscale(n, -1.0f);
    }
    
    *inside = false;
    
    return n;
}

static float directDiffuse(const Material *mat, Vector3f hitPos, Vector3f normal, const Sphere *ignore, int wl) {
    float dif = mat->diffuse.spectrum[wl];
    
    if (dif <= 0.0f) {
        return 0.0f;
    }

    float total = 0.0f;
    
    for (int l = 0; l < NUM_LIGHTS; l++) {
        Vector3f L = vsub(lights[l].position, hitPos);
        float dist2 = vlen2(L);
        
        if (dist2 <= epsilon * epsilon) {
            continue;
        }

        float dist = sqrtf(dist2);
        Vector3f lightDir = vscale(L, 1.0f / dist);

        if (inShadow(vadd(hitPos, vscale(normal, epsilon)), lightDir, dist2, ignore)) {
            continue;
        }

        float attenuation = 1.0f / dist2;
        float lambert = fmaxf(0.0f, vdot(normal, lightDir));
        total += dif * lambert * lights[l].intensity * attenuation * lights[l].color->spectrum[wl];
    }
    
    return total;
}

static Vector3f sampleCosineHemisphere(Vector3f normal, int idx, int seed) {
    float u1 = randFloat(idx, seed);
    float u2 = randFloat(idx, seed + 1);
    float r = sqrtf(u1);
    float phi = 2.0f * M_PI * u2;
    
    Vector3f axis = (fabsf(normal.x) > 0.9f) ? vec3(0,1,0) : vec3(1,0,0);
    Vector3f u = vnorm(vcross(axis, normal));
    Vector3f v = vcross(normal, u);
    Vector3f d = vadd(vscale(u, r * cosf(phi)), vadd(vscale(v, r * sinf(phi)), vscale(normal, sqrtf(1.0f - u1))));
    
    return vnorm(d);
}

static TraceResult traceRay(Vector3f ro, Vector3f rd, int depth, int wl, int idx, int frame);

static TraceResult traceRough(float coneFractionCache[], Vector3f reflOrig, Vector3f axis, Vector3f normal, float roughness, float weight, int depth, int wl, int idx, int frame) {
    TraceResult result = {0.0f, false};
    int baseSeed = randInt(idx ^ (uint32_t)frame);

    // Cache coneFraction once per pixel (first path to fire wins)
    if (depth == 0 && coneFractionCache[0] < -0.5f) {
        float theta = roughness * M_PI_2;
        float total = 0.0f;
        
        for (int s = 0; s < NUM_SPHERES; s++) {
            total += findConeCoverage(reflOrig, axis, theta, spheres[s].position, spheres[s].radius);
        }
        
        coneFractionCache[0] = (total > 1.0f) ? 1.0f : total;
    }
    
    float coneCoverage = (depth == 0) ? coneFractionCache[0] : -1.0f;

    if (coneCoverage >= 0.0f && coneCoverage < 0.01f) {
        // Cone is essentially all sky: analytic shortcut
        result.intensity = weight * skySpectrum.spectrum[wl];
        result.deterministic = true;
        
        return result;
    }

    if (coneCoverage < 0.0f) {
        // No analytic estimate: single unbiased sample
        Vector3f dir = roughenDirection(axis, normal, roughness, idx, baseSeed);
        TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
        result.intensity = weight * sub.intensity;
        result.deterministic = sub.deterministic;
        
        return result;
    }

    // Rejection sampling: try to land on a content sphere
    Vector3f dir = axis;
    bool foundSphereHit = false;
    int tries = 4 + (int)(3.0f / coneCoverage);       //TODO: Find best scalar (3.0f ?)
    
    if (tries > 200) {
        tries = 200;        // Hard cap of 200 attempts
    }

    for (int t = 0; t < tries && !foundSphereHit; t++) {
        dir = roughenDirection(axis, normal, roughness, idx, baseSeed + t * 7777);     // TODO: Find better seed calculation (frame counter ?)
        
        for (int s = 1; s < NUM_SPHERES && !foundSphereHit; s++) {
            Hit is = intersectSphere(&spheres[s], reflOrig, dir);
            
            if (is.hit && is.t > epsilon) {
                foundSphereHit = true;
                break;
            }
        }
    }

    float skyPart = (1.0f - coneCoverage) * skySpectrum.spectrum[wl];
    
    if (foundSphereHit) {
        TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
        result.intensity = weight * (skyPart + coneCoverage * sub.intensity);
        result.deterministic = sub.deterministic;
    } else {
        // Rare: fallback to unbiased single sample
        dir = roughenDirection(axis, normal, roughness, idx, baseSeed + 999);
        TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
        result.intensity = weight * sub.intensity;
        result.deterministic = sub.deterministic;
    }
    
    result.deterministic = false;       // All rough rays are stochastic therefore not deterministic
    
    return result;
}

static TraceResult traceRay(Vector3f ro, Vector3f rd, int depth, int wl, int idx, int frame) {
    // 1. Find closest hit
    float t;
    const Sphere *hit = closestHit(ro, rd, &t);
    
    if (!hit) {
        return (TraceResult){skySpectrum.spectrum[wl], true};
    }

    // 2. Surface geometry
    bool inside;
    Vector3f hitPos = vadd(ro, vscale(rd, t));
    Vector3f normal = surfaceNormal(hitPos, hit, rd, &inside);

    // 3. Material parameters
    const Material *mat = hit->material;
    float dif = mat->diffuse.spectrum[wl];
    float spec = mat->specular.spectrum[wl];
    float trans = mat->transmission.spectrum[wl];
    float roughness = mat->roughness;

    // 4. Direct diffuse from point lights
    float intensity = directDiffuse(mat, hitPos, normal, hit, wl);

    // 5. Depth limit — assume escaped to sky
    if (depth >= MAX_RAY_DEPTH) {
        return (TraceResult){intensity + skySpectrum.spectrum[wl], true};
    }

    // 6. If nothing to sample further, done
    bool wantDiffuseGI = (dif > 0.0f && roughness > 0.0f);
    bool wantSpecular = (spec > 0.0f);
    bool wantTrans = (trans > 0.0f);
    
    if (!wantDiffuseGI && !wantSpecular && !wantTrans) {
        return (TraceResult){intensity, true};
    }

    // 7. Interface properties
    float cosi = fabsf(vdot(rd, normal));
    float n1 = inside ? mat->ior.spectrum[wl] : 1.0f;
    float n2 = inside ? 1.0f : mat->ior.spectrum[wl];
    float eta = n1 / n2;
    float k = 1.0f - eta * eta * (1.0f - cosi * cosi);

    Vector3f reflDir = reflectDir(rd, normal);
    Vector3f reflOrig = vadd(hitPos, vscale(normal, epsilon));
    Vector3f refrOrig = vsub(hitPos, vscale(normal, epsilon));

    bool deterministic = true;
    int baseSeed = randInt(idx ^ (uint32_t)frame);

    // 8. Branch on interface case
    if (k >= 0.0f) {
        // Normal interface: Fresnel-split reflection + refraction
        float fresnel = fresnelDielectric(cosi, n1, n2);

        if (wantSpecular) {
            if (roughness > 0.0f) {
                deterministic = false;
            }
        
            Vector3f dir = roughenDirection(reflDir, normal, roughness, idx, baseSeed);
            TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
            intensity += fresnel * spec * sub.intensity;
            deterministic &= sub.deterministic;
        }
        if (wantTrans) {
            if (roughness > 0.0f) {
                deterministic = false;
            }
        
            Vector3f dir = refractDir(rd, normal, eta, cosi, k);
            dir = roughenDirection(dir, normal, roughness, idx, baseSeed + 1000);
            TraceResult sub = traceRay(refrOrig, dir, depth + 1, wl, idx, frame);
            intensity += (1.0f - fresnel) * trans * sub.intensity;
            deterministic &= sub.deterministic;
        }
    } else {
        // k < 0: TIR, or ior = 0 (opaque mirror / matte)
        if (wantTrans) {
            // TIR: all energy reflects (energy-conserving)
            Vector3f dir = roughenDirection(reflDir, normal, roughness, idx, baseSeed);
            TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
            intensity += sub.intensity;  // weight = 1.0
            deterministic &= sub.deterministic;
        } else if (wantSpecular) {
            if (roughness <= 0.0f) {
                // Smooth opaque mirror
                TraceResult sub = traceRay(reflOrig, reflDir, depth + 1, wl, idx, frame);
                intensity += spec * sub.intensity;
                deterministic &= sub.deterministic;
            } else {
                // Rough opaque specular
                TraceResult sub = traceRough(&pixels[idx].coneFractionSpecular, reflOrig, reflDir, normal, roughness, spec, depth, wl, idx, frame);
                intensity += sub.intensity;
                deterministic &= sub.deterministic;
            }
        } else if (wantDiffuseGI) {
            // Cosine-weighted diffuse GI with rejection sampling.
            // Cache f_cos once per pixel (reuses coneFractionDiffuse slot).
            if (depth == 0 && pixels[idx].coneFractionDiffuse < -0.5f) {
                float total = 0.0f;
                
                for (int s = 1; s < NUM_SPHERES; s++) {
                    total += cosineCoverage(reflOrig, normal, spheres[s].position, spheres[s].radius);
                }
                
                pixels[idx].coneFractionDiffuse = (total > 1.0f) ? 1.0f : total;
            }
            
            float fCos = (depth == 0) ? pixels[idx].coneFractionDiffuse : -1.0f;

            if (fCos >= 0.0f && fCos < 0.01f) {
                intensity += dif * skySpectrum.spectrum[wl];        // Essentially all sky
            } else if (fCos < 0.0f) {
                Vector3f dir = sampleCosineHemisphere(normal, idx, baseSeed);
                TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
                intensity += dif * sub.intensity;
                deterministic = false;
            } else {
                Vector3f dir = normal;          // Sample until hit
                bool found = false;
                int tries = 4 + (int)(3.0f / fCos);
                
                if (tries > 200) tries = 200;

                for (int t = 0; t < tries && !found; t++) {
                    dir = sampleCosineHemisphere(normal, idx, baseSeed + t * 911);         // TODO: Find better seed formula
                    for (int s = 1; s < NUM_SPHERES && !found; s++) {
                        Hit is = intersectSphere(&spheres[s], reflOrig, dir);
                        if (is.hit && is.t > epsilon) found = true;
                    }
                }

                float skyPart = (1.0f - fCos) * skySpectrum.spectrum[wl];

                if (found) {
                    TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
                    intensity += dif * (skyPart + fCos * sub.intensity);
                    deterministic &= sub.deterministic;
                } else {
                    // Fallback: take one plain cosine sample (unbiased)
                    dir = sampleCosineHemisphere(normal, idx, baseSeed + 32771);      // TODO: Find better seed calculation
                    TraceResult sub = traceRay(reflOrig, dir, depth + 1, wl, idx, frame);
                    intensity += dif * sub.intensity;
                }

                deterministic = false;   // cosine sampling happened
            }
        }
    }

    return (TraceResult){intensity, deterministic};
}

static inline uint16_t floatToRGB565(float r, float g, float b) {
    // Discretize from float 0-1 to int 0-NUM_VALUES
    int rInt = (int)(r * 4095.0f);
    int gInt = (int)(g * 4095.0f);
    int bInt = (int)(b * 4095.0f);
    
    //  Clamp channels to 0-NUM_GAMMA_VALUES range
    if (rInt < 0) {
        rInt = 0;
    } else if (rInt > NUM_GAMMA_VALUES) {
        rInt = NUM_GAMMA_VALUES;
    }
    
    if (gInt < 0) {
        gInt = 0;
    } else if (gInt > NUM_GAMMA_VALUES) {
        gInt = NUM_GAMMA_VALUES;
    }
    
    if (bInt < 0) {
        bInt = 0;
    } else if (bInt > NUM_GAMMA_VALUES) {
        bInt = NUM_GAMMA_VALUES;
    }

    // Fit all 3 into 16-bit RGB565 number
    return ((gammaLUT[rInt] >> 3) << 11) | ((gammaLUT[gInt] >> 2) << 5) | (gammaLUT[bInt] >> 3);
}

static inline float toneMap(float x) {
    if (x <= 0.0f) {
        return 0.0f;
    }
    
    x *= DISPLAY_EXPOSURE;
    
    return x / (1.0f + x);
}

static inline void xyzToDisplay(float X, float Y, float Z, float *r, float *g, float *b) {
    X *= invYIntegral;
    Y *= invYIntegral;
    Z *= invYIntegral;
    
    *r = toneMap( 3.2406f*X - 1.5372f*Y - 0.4986f*Z);
    *g = toneMap(-0.9689f*X + 1.8758f*Y + 0.0415f*Z);
    *b = toneMap( 0.0557f*X - 0.2040f*Y + 1.0570f*Z);
}

static inline void toneMapPixel(uint32_t idx) {
    uint32_t n = pixels[idx].samples;
    
    if (n == 0) {
        return;
    }
    
    float inv = 1.0f / (float)n;
    float r, g, b;
    
    xyzToDisplay(pixels[idx].accumR * inv, pixels[idx].accumG * inv, pixels[idx].accumB * inv, &r, &g, &b);
    screenBuffer[idx] = floatToRGB565(r, g, b);
}

static void flushAndDisplay(void) {
    for (size_t i = 0; i < numActive; i++) {
        toneMapPixel(activePixels[i]);
    }
    
    plat_blit(screenBuffer);
}

int main(void) {
    plat_init();
    screenBuffer = (uint16_t*) malloc(SCR_W * SCR_H * sizeof(uint16_t));
    
    if (!screenBuffer) {
        return 1;
    }

    initTables();
    initGammaLUT();
    initSkyRGB();
    initScene();
    initRays();
    
    float sr, sg, sb;
    xyzToDisplay(skyXYZ[0], skyXYZ[1], skyXYZ[2], &sr, &sg, &sb);
    uint16_t sky565 = floatToRGB565(sr, sg, sb);

    for (uint32_t idx = 0; idx < SCR_W * SCR_H; idx++) {
        pixels[idx].coneFractionDiffuse = -1.0f;      // <= 0.0f is uninitialized
        pixels[idx].coneFractionSpecular = -1.0f;     // ''
        pixels[idx].accumR = 0.0f;
        pixels[idx].accumG = 0.0f;
        pixels[idx].accumB = 0.0f;
        screenBuffer[idx] = sky565;
    }
    
    numActive = (size_t)SCR_W * SCR_H;
    activePixels = malloc(numActive * sizeof(uint32_t));
    
    if (!activePixels) {
        free(screenBuffer);
        plat_cleanup();
        
        return 1;
    }
    
    for (size_t i = 0; i < numActive; i++) {
        activePixels[i] = (uint32_t)i;
    }

    plat_blit(screenBuffer);

    int frame = 0;
    
    uint64_t t0 = plat_ticks();
    uint64_t total_traces = 0;
    
    while (!plat_quit() && numActive > 0) {
        total_traces += numActive;
        
        for (size_t i = 0; i < numActive; i++) {
            uint32_t idx = activePixels[i];
            
            if (pixels[idx].samples >= MAX_SAMPLES_PER_PIXEL) {
                pixels[idx].converged = true;
                continue;
            }
            
            Vector3f ro = {0, 0, 0};
            Vector3f rd = rayDir[idx];

            bool deterministic = true;
            
            for (int wl = 0; wl < NUM_WAVELENGTHS; wl++) {
                TraceResult tr = traceRay(ro, rd, 0, wl, idx, frame);
                
                pixels[idx].accumR += tr.intensity * xyz[wl][0] * WAVELENGTH_WEIGHT;
                pixels[idx].accumG += tr.intensity * xyz[wl][1] * WAVELENGTH_WEIGHT;
                pixels[idx].accumB += tr.intensity * xyz[wl][2] * WAVELENGTH_WEIGHT;
                
                deterministic &= tr.deterministic;
            }
            
            pixels[idx].samples++;
            
            float inv = 1.0f / (float)pixels[idx].samples;
            float newR = pixels[idx].accumR * inv;
            float newG = pixels[idx].accumG * inv;
            float newB = pixels[idx].accumB * inv;
            
            bool converged = deterministic;
            
            if (!converged && pixels[idx].samples >= 2) {
                float dR = fabsf(newR - pixels[idx].lastR);
                float dG = fabsf(newG - pixels[idx].lastG);
                float dB = fabsf(newB - pixels[idx].lastB);
                float dMax = fmaxf(dR, fmaxf(dG, dB));

                // Also check relative for bright pixels
                float rMax = fmaxf(fabsf(pixels[idx].lastR), fmaxf(fabsf(pixels[idx].lastG), fabsf(pixels[idx].lastB)));
                float relChange = dMax / (rMax + epsilon);

                if (dMax < 0.005f || relChange < DELTA_THRESHOLD) {
                    if (++pixels[idx].stableCount >= CONVERGE_FRAMES) {
                        converged = true;
                    }
                } else {
                    pixels[idx].stableCount = 0;
                }
            }
            
            pixels[idx].lastR = newR;
            pixels[idx].lastG = newG;
            pixels[idx].lastB = newB;

            if (converged) {
                pixels[idx].converged = true;
            }
        }
        
        size_t w = 0;
            
        for (size_t i = 0; i < numActive; i++) {
            uint32_t j = activePixels[i];
            
            if (pixels[j].converged) {
                toneMapPixel(j);          // final write for this pixel
            } else {
                activePixels[w++] = j;
            }
        }
        
        numActive = w;
        
        if (frame % 10 == 0) {
            printf("\nframe: %4u | active: %6u", (unsigned)frame, (unsigned)numActive);
            fflush(stdout);
        }
        
        flushAndDisplay();
        frame++;
    }
    
    uint64_t t1 = plat_ticks();

    printf("\n=== BENCH ===\n");
    printf("frames:       %d\n", frame);
    printf("total ticks:  %llu\n", (unsigned long long)(t1 - t0));
    printf("ticks/frame:  %llu\n", (unsigned long long)((t1 - t0) / frame));
    printf("total traces: %llu\n", (unsigned long long)total_traces);
    printf("final active: %u\n", (unsigned)numActive);

    plat_save_ppm(screenBuffer, SCR_W, SCR_H, "output.ppm");
    plat_cleanup();
    
    return 0;
}

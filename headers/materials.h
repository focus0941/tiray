#ifndef MATERIALS_H
#define MATERIALS_H

#define NUM_WAVELENGTHS 8

typedef struct { float spectrum[NUM_WAVELENGTHS]; } Spectrum;

typedef struct {
    Spectrum diffuse;
    Spectrum specular;
    Spectrum transmission;
    Spectrum ior;
    float roughness;
} Material;

static inline Material newMaterial(Spectrum d, Spectrum s, Spectrum t, Spectrum i, float r) {
    Material m = {d, s, t, i, r};
    return m;
}

#define FULL_SPECTRUM {{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}}
static const Spectrum FULL_SPECTRUM_VAR = FULL_SPECTRUM;
#define ZERO_SPECTRUM {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}}

#define WHITE_SPECTRUM {{0.7994f, 0.7994f, 0.7994f, 0.7994f, 0.7994f, 0.7994f, 0.7994f, 0.7994f}}
#define LIGHT_GRAY_SPECTRUM {{0.3324f, 0.3324f, 0.3324f, 0.3324f, 0.3324f, 0.3324f, 0.3324f, 0.3324f}}
#define DARK_GRAY_SPECTRUM {{0.0717f, 0.0717f, 0.0717f, 0.0717f, 0.0717f, 0.0717f, 0.0717f, 0.0717f}}
#define BLACK_SPECTRUM {{0.0163f, 0.0163f, 0.0163f, 0.0163f, 0.0163f, 0.0163f, 0.0163f, 0.0163f}}
#define BROWN_SPECTRUM {{0.0609f, 0.0601f, 0.0662f, 0.0976f, 0.1624f, 0.1571f, 0.1519f, 0.1522f}}
#define RED_SPECTRUM {{0.0799f, 0.0765f, 0.0760f, 0.1021f, 0.3829f, 0.6299f, 0.6468f, 0.6660f}}
#define ORANGE_SPECTRUM {{0.0415f, 0.0432f, 0.0633f, 0.1751f, 0.5265f, 0.6300f, 0.6502f, 0.6679f}}
#define YELLOW_SPECTRUM {{0.0402f, 0.0587f, 0.3027f, 0.6203f, 0.6520f, 0.6690f, 0.6818f, 0.6974f}}
#define LIGHT_GREEN_SPECTRUM {{0.2084f, 0.2775f, 0.5304f, 0.5574f, 0.4007f, 0.2925f, 0.3165f, 0.3431f}}
#define LIME_SPECTRUM {{0.1439f, 0.1818f, 0.4899f, 0.5751f, 0.4104f, 0.3001f, 0.3194f, 0.3448f}}
#define DARK_GREEN_SPECTRUM {{0.0583f, 0.0649f, 0.1217f, 0.1210f, 0.0890f, 0.0706f, 0.0744f, 0.0837f}}
#define LIGHT_BLUE_SPECTRUM {{0.3396f, 0.5610f, 0.6000f, 0.3652f, 0.1951f, 0.1741f, 0.1712f, 0.2913f}}
#define CYAN_SPECTRUM {{0.2676f, 0.4191f, 0.5565f, 0.3674f, 0.1882f, 0.1531f, 0.1669f, 0.2083f}}
#define TEAL_SPECTRUM {{0.1768f, 0.2682f, 0.5031f, 0.4146f, 0.1957f, 0.1286f, 0.1539f, 0.1819f}}
#define DARK_BLUE_SPECTRUM {{0.2148f, 0.3142f, 0.1722f, 0.0867f, 0.0734f, 0.0904f, 0.0962f, 0.1673f}}
#define PURPLE_SPECTRUM {{0.2516f, 0.2645f, 0.1323f, 0.0867f, 0.1100f, 0.2259f, 0.4835f, 0.6063f}}
#define MAGENTA_SPECTRUM {{0.2641f, 0.2656f, 0.1536f, 0.0995f, 0.2860f, 0.5920f, 0.6519f, 0.6894f}}
#define PINK_SPECTRUM {{0.2996f, 0.3724f, 0.3336f, 0.3352f, 0.4708f, 0.4770f, 0.4701f, 0.4651f}}

#define GOLD_SPECTRUM {{0.4071f, 0.4082f, 0.4791f, 0.7909f, 0.9074f, 0.9556f, 0.9704f, 0.9741f}}
#define SILVER_SPECTRUM {{0.9637f, 0.9802f, 0.9816f, 0.9830f, 0.9872f, 0.9898f, 0.9932f, 0.9955f}}
#define COPPER_SPECTRUM {{0.4689f, 0.5382f, 0.5783f, 0.6239f, 0.8154f, 0.9336f, 0.9548f, 0.9585f}}

#define GLASS_IOR {{1.5309f, 1.5253f, 1.5214f, 1.5185f, 1.5163f, 1.5145f, 1.5132f, 1.5121f}}
#define DIAMOND_IOR {{2.5118f, 2.4904f, 2.4751f, 2.4638f, 2.4552f, 2.4485f, 2.4432f, 2.4389f}}
#define WATER_IOR {{1.3434f, 1.3393f, 1.3364f, 1.3342f, 1.3326f, 1.3313f, 1.3303f, 1.3295f}}

#endif


#ifndef MATERIALS_H
    #define MATERIALS_H

#ifdef NSPIRE
    #define NUM_WAVELENGTHS 3
#else
    #define NUM_WAVELENGTHS 8
#endif

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

#ifdef NSPIRE
    #define FULL_SPECTRUM       {{1.0000f, 1.0000f, 1.0000f}}
    #define ZERO_SPECTRUM       {{0.0000f, 0.0000f, 0.0000f}}
    #define WHITE_SPECTRUM      {{1.0000f, 1.0000f, 1.0000f}}
    #define LIGHT_GRAY_SPECTRUM {{0.6000f, 0.6000f, 0.6000f}}
    #define DARK_GRAY_SPECTRUM  {{0.3000f, 0.3000f, 0.3000f}}
    #define BLACK_SPECTRUM      {{0.0200f, 0.0200f, 0.0200f}}

    #define RED_SPECTRUM {{0.0003f, 0.1353f, 1.0000f}}
    #define ORANGE_SPECTRUM {{0.0111f, 0.6065f, 0.6065f}}
    #define YELLOW_SPECTRUM {{0.0051f, 0.7548f, 0.2163f}}
    #define LIME_SPECTRUM {{0.0228f, 0.9692f, 0.0796f}}
    #define LIGHT_GREEN_SPECTRUM {{0.1353f, 0.8825f, 0.0111f}}
    #define DARK_GREEN_SPECTRUM {{0.6065f, 0.6065f, 0.0111f}}
    #define CYAN_SPECTRUM {{0.7548f, 0.2163f, 0.0001f}}
    #define LIGHT_BLUE_SPECTRUM {{0.9692f, 0.0796f, 0.0000f}}
    #define DARK_BLUE_SPECTRUM {{1.0000f, 0.0439f, 0.0000f}}
    #define PURPLE_SPECTRUM {{0.7548f, 0.0051f, 0.0000f}}
    #define BROWN_SPECTRUM {{0.0012f, 0.1501f, 0.3341f}}
    #define MAGENTA_SPECTRUM {{0.4578f, 0.0018f, 0.4578f}}
    #define PINK_SPECTRUM {{1.0000f, 0.6505f, 0.6065f}}

    #define GOLD_SPECTRUM   {{0.4082f, 0.7909f, 0.9556f}}
    #define SILVER_SPECTRUM {{0.9802f, 0.9830f, 0.9898f}}
    #define COPPER_SPECTRUM {{0.5382f, 0.6239f, 0.9336f}}

    #define GLASS_IOR   {{1.5253f, 1.5185f, 1.5145f}}
    #define DIAMOND_IOR {{2.4904f, 2.4638f, 2.4485f}}
    #define WATER_IOR   {{1.3393f, 1.3342f, 1.3313f}}
#else
    #define FULL_SPECTRUM       {{1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f}}
    #define ZERO_SPECTRUM       {{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}}
    #define WHITE_SPECTRUM      {{1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f, 1.0000f}}
    #define LIGHT_GRAY_SPECTRUM {{0.6000f, 0.6000f, 0.6000f, 0.6000f, 0.6000f, 0.6000f, 0.6000f, 0.6000f}}
    #define DARK_GRAY_SPECTRUM  {{0.3000f, 0.3000f, 0.3000f, 0.3000f, 0.3000f, 0.3000f, 0.3000f, 0.3000f}}
    #define BLACK_SPECTRUM      {{0.0200f, 0.0200f, 0.0200f, 0.0200f, 0.0200f, 0.0200f, 0.0200f, 0.0200f}}

    #define RED_SPECTRUM {{0.0000f, 0.0003f, 0.0111f, 0.1353f, 0.6065f, 1.0000f, 0.6065f, 0.1353f}}
    #define ORANGE_SPECTRUM {{0.0003f, 0.0111f, 0.1353f, 0.6065f, 1.0000f, 0.6065f, 0.1353f, 0.0111f}}
    #define YELLOW_SPECTRUM {{0.0000f, 0.0051f, 0.1353f, 0.7548f, 0.8825f, 0.2163f, 0.0111f, 0.0001f}}
    #define LIME_SPECTRUM {{0.0003f, 0.0228f, 0.3247f, 0.9692f, 0.6065f, 0.0796f, 0.0022f, 0.0000f}}
    #define LIGHT_GREEN_SPECTRUM {{0.0051f, 0.1353f, 0.7548f, 0.8825f, 0.2163f, 0.0111f, 0.0001f, 0.0000f}}
    #define DARK_GREEN_SPECTRUM {{0.1353f, 0.6065f, 1.0000f, 0.6065f, 0.1353f, 0.0111f, 0.0003f, 0.0000f}}
    #define CYAN_SPECTRUM {{0.1353f, 0.7548f, 0.8825f, 0.2163f, 0.0111f, 0.0001f, 0.0000f, 0.0000f}}
    #define LIGHT_BLUE_SPECTRUM {{0.3247f, 0.9692f, 0.6065f, 0.0796f, 0.0022f, 0.0000f, 0.0000f, 0.0000f}}
    #define DARK_BLUE_SPECTRUM {{0.4578f, 1.0000f, 0.4578f, 0.0439f, 0.0009f, 0.0000f, 0.0000f, 0.0000f}}
    #define PURPLE_SPECTRUM {{0.8825f, 0.7548f, 0.1353f, 0.0051f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}}
    #define BROWN_SPECTRUM {{0.0000f, 0.0012f, 0.0225f, 0.1501f, 0.3692f, 0.3341f, 0.1112f, 0.0136f}}
    #define MAGENTA_SPECTRUM {{1.0000f, 0.4578f, 0.0439f, 0.0018f, 0.0439f, 0.4578f, 1.0000f, 0.4578f}}
    #define PINK_SPECTRUM {{0.4582f, 1.0000f, 0.5932f, 0.6505f, 1.0000f, 0.6065f, 0.1353f, 0.0111f}}

    #define GOLD_SPECTRUM   {{0.4071f, 0.4082f, 0.4791f, 0.7909f, 0.9074f, 0.9556f, 0.9704f, 0.9741f}}
    #define SILVER_SPECTRUM {{0.9637f, 0.9802f, 0.9816f, 0.9830f, 0.9872f, 0.9898f, 0.9932f, 0.9955f}}
    #define COPPER_SPECTRUM {{0.4689f, 0.5382f, 0.5783f, 0.6239f, 0.8154f, 0.9336f, 0.9548f, 0.9585f}}

    #define GLASS_IOR   {{1.5309f, 1.5253f, 1.5214f, 1.5185f, 1.5163f, 1.5145f, 1.5132f, 1.5121f}}
    #define DIAMOND_IOR {{2.5118f, 2.4904f, 2.4751f, 2.4638f, 2.4552f, 2.4485f, 2.4432f, 2.4389f}}
    #define WATER_IOR   {{1.3434f, 1.3393f, 1.3364f, 1.3342f, 1.3326f, 1.3313f, 1.3303f, 1.3295f}}
#endif

static const Spectrum FULL_SPECTRUM_VAR = FULL_SPECTRUM;

#endif

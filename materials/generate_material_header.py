#!/usr/bin/env python3
import math
import csv
import os
import sys

WL_MIN = 400.0
WL_MAX = 750.0
NUM_WL_LAPTOP = 8
NUM_WL_NSPIRE = 3
NSPIRE_WLS = [450.0, 550.0, 650.0]

def get_wavelengths(min_wl, max_wl, count):
    if count == 3:
        return NSPIRE_WLS
    
    step = (max_wl - min_wl) / (count - 1)
    
    return [min_wl + i * step for i in range(count)]

def gaussian(center, width, amplitude=1.0):
    def func(wl):
        val = amplitude * math.exp(-0.5 * ((wl - center) / width)**2)
        
        return min(1.0, val)
    
    return func

def format_spectrum(vals):
    return "{{" + ", ".join(f"{v:.4f}f" for v in vals) + "}}"

def load_metal_csv(filename, target_wavelengths):
    if not os.path.exists(filename):
        print(f"// WARNING: {filename} not found, using fallback 0.9", file=sys.stderr)
        
        return [0.9] * len(target_wavelengths)

    wls = []
    refl = []
    
    with open(filename, 'r') as f:
        reader = csv.reader(f)
        next(reader, None) # skip header
        
        for row in reader:
            if len(row) >= 2:
                # Assuming standard format: wavelength in micrometers
                wls.append(float(row[0]) * 1000.0)
                refl.append(float(row[1]))

    # Sort by wavelength just in case
    sorted_pairs = sorted(zip(wls, refl))
    wls = [p[0] for p in sorted_pairs]
    refl = [p[1] for p in sorted_pairs]

    # Interpolate to target wavelengths
    out = []
    
    for tw in target_wavelengths:
        if tw <= wls[0]:
            out.append(refl[0])
        elif tw >= wls[-1]:
            out.append(refl[-1])
        else:
            # lerp
            for i in range(len(wls) - 1):
                if wls[i] <= tw <= wls[i+1]:
                    t = (tw - wls[i]) / (wls[i+1] - wls[i])
                    val = refl[i] * (1 - t) + refl[i+1] * t
                    out.append(min(1.0, max(0.0, val)))
                    break
    return out

def generate_macros(wavelengths, indent=""):
    macros = []
    
    def eval_spectrum(func):
        if callable(func):
            return [func(w) for w in wavelengths]
        
        return func

    # --- NEUTRALS ---
    macros.append(f"    #define FULL_SPECTRUM       {format_spectrum([1.0]*len(wavelengths))}")
    macros.append(f"    #define ZERO_SPECTRUM       {format_spectrum([0.0]*len(wavelengths))}")
    macros.append(f"    #define WHITE_SPECTRUM      {format_spectrum([1.0]*len(wavelengths))}")
    macros.append(f"    #define LIGHT_GRAY_SPECTRUM {format_spectrum([0.6]*len(wavelengths))}")
    macros.append(f"    #define DARK_GRAY_SPECTRUM  {format_spectrum([0.3]*len(wavelengths))}")
    macros.append(f"    #define BLACK_SPECTRUM      {format_spectrum([0.02]*len(wavelengths))}\n")

    # --- GAUSSIAN COLORS ---
    colors = {
        "RED":         gaussian(650, 50),
        "ORANGE":      gaussian(600, 50),
        "YELLOW":      gaussian(580, 40),
        "LIME":        gaussian(560, 40),
        "LIGHT_GREEN": gaussian(530, 40),
        "DARK_GREEN":  gaussian(500, 50),
        "CYAN":        gaussian(480, 40),
        "LIGHT_BLUE":  gaussian(460, 40),
        "DARK_BLUE":   gaussian(450, 40),
        "PURPLE":      gaussian(420, 40),
        "BROWN":       gaussian(620, 50, 0.4),
    }
    
    def dual_peak(c1, w1, c2, w2):
        f1 = gaussian(c1, w1)
        f2 = gaussian(c2, w2)
        
        return lambda wl: min(1.0, f1(wl) + f2(wl))

    colors["MAGENTA"] = dual_peak(400, 40, 700, 40)
    colors["PINK"] = dual_peak(450, 40, 600, 50)

    for name, func in colors.items():
        vals = eval_spectrum(func)
        macros.append(f"    #define {name}_SPECTRUM {format_spectrum(vals)}")
    
    macros.append("")

    # Metals from .csv
    gold_vals   = load_metal_csv("gold.csv", wavelengths)
    silver_vals = load_metal_csv("silver.csv", wavelengths)
    copper_vals = load_metal_csv("copper.csv", wavelengths)

    macros.append(f"    #define GOLD_SPECTRUM   {format_spectrum(gold_vals)}")
    macros.append(f"    #define SILVER_SPECTRUM {format_spectrum(silver_vals)}")
    macros.append(f"    #define COPPER_SPECTRUM {format_spectrum(copper_vals)}\n")

    # IORs from Cauchy equations
    glass_ior   = [1.5046 + 4200.0 / (w**2) for w in wavelengths]
    diamond_ior = [2.4099 + 16300.0 / (w**2) for w in wavelengths]
    water_ior   = [1.3240 + 3100.0 / (w**2) for w in wavelengths]

    macros.append(f"    #define GLASS_IOR   {format_spectrum(glass_ior)}")
    macros.append(f"    #define DIAMOND_IOR {format_spectrum(diamond_ior)}")
    macros.append(f"    #define WATER_IOR   {format_spectrum(water_ior)}")

    return "\n".join(macros)

def main():
    print("#ifndef MATERIALS_H")
    print("    #define MATERIALS_H\n")

    print("#ifdef NSPIRE")
    print("    #define NUM_WAVELENGTHS 3")
    print("#else")
    print("    #define NUM_WAVELENGTHS 8")
    print("#endif\n")

    print("typedef struct { float spectrum[NUM_WAVELENGTHS]; } Spectrum;\n")
    
    print("typedef struct {")
    print("    Spectrum diffuse;")
    print("    Spectrum specular;")
    print("    Spectrum transmission;")
    print("    Spectrum ior;")
    print("    float roughness;")
    print("} Material;\n")

    print("static inline Material newMaterial(Spectrum d, Spectrum s, Spectrum t, Spectrum i, float r) {")
    print("    Material m = {d, s, t, i, r};")
    print("    return m;")
    print("}\n")

    # Generate Nspire block
    print("#ifdef NSPIRE")
    nspire_wls = get_wavelengths(WL_MIN, WL_MAX, NUM_WL_NSPIRE)
    print(generate_macros(nspire_wls, indent="  "))
    
    # Generate Laptop block
    print("#else")
    laptop_wls = get_wavelengths(WL_MIN, WL_MAX, NUM_WL_LAPTOP)
    print(generate_macros(laptop_wls, indent="  "))
    print("#endif\n")

    print("static const Spectrum FULL_SPECTRUM_VAR = FULL_SPECTRUM;\n")
    print("#endif")

if __name__ == "__main__":
    main()

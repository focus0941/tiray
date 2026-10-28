#ifndef PLATFORM_LAPTOP_H
#define PLATFORM_LAPTOP_H

#include <stdio.h>
#include <time.h>

static inline void plat_init(void) {}

static inline void plat_blit(uint16_t *buf) {          // headless
    (void)buf;
}

static inline bool plat_quit(void) {
    return false;
}

static inline uint64_t plat_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline void plat_save_ppm(uint16_t *buf, int w, int h, const char *path) {
    FILE *f = fopen(path, "wb");
    
    if (!f) {
        return;
    }
    
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    
    for (int i = 0; i < w * h; i++) {
        uint16_t c = buf[i];
        uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
        uint8_t g = (uint8_t)(((c >> 5)  & 0x3F) * 255 / 63);
        uint8_t b = (uint8_t)(( c        & 0x1F) * 255 / 31);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    
    fclose(f);
}

static inline void plat_cleanup(void) {}

#endif

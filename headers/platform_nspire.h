#ifndef PLATFORM_NSPIRE_H
#define PLATFORM_NSPIRE_H

#include <os.h>
#include <libndls.h>

static inline void plat_init(void) {
    lcd_init(SCR_320x240_565);
}

static inline void plat_blit(uint16_t *buf) {
    lcd_blit(buf, SCR_320x240_565);
}

static inline bool plat_quit(void) {
    return isKeyPressed(KEY_NSPIRE_ESC);
}

static inline void plat_cleanup(void) {
    lcd_init(SCR_TYPE_INVALID);
}

static inline void plat_save_ppm(uint16_t *b, int w, int h, const char *p) {
    (void)b;
    (void)w;
    (void)h;
    (void)p;
}

static inline uint64_t plat_ticks(void) {
    static uint32_t last = 0;
    static uint64_t base = 0;
    uint32_t now = *(volatile uint32_t*)0x90010004;
    // SP804 is a DOWN-counter, so elapsed = last - now (unsigned wraps correctly)
    base += (uint32_t)(last - now);
    last = now;
    
    return base;
}

#endif

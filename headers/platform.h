#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef NSPIRE
  #include "platform_nspire.h"
  #define SCR_W 320
  #define SCR_H 240
#else
  #include "platform_laptop.h"
  #define SCR_W 320//1920
  #define SCR_H 240//1080
#endif



#endif

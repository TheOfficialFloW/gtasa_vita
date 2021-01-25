#ifndef __CONFIG_H__
#define __CONFIG_H__

#define LOAD_ADDRESS 0x98000000

#define MEMORY_MB 280

#define DATA_PATH "ux0:data/gtasa"
#define SO_PATH DATA_PATH "/" "libGTASA.so"

#define SCREEN_W 960
#define SCREEN_H 544

#define DISABLE_SPEC_AMT
#define DISABLE_ALPHA_TESTING
#define OPTIMIZE_ALPHA_MODULATION
#define OPTIMIZE_MVP
#define SLOW_GPU

#endif

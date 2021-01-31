#ifndef __CONFIG_H__
#define __CONFIG_H__

#define LOAD_ADDRESS 0x98000000

#define MEMORY_MB 256

#define DATA_PATH "ux0:data/gtasa"
#define SO_PATH DATA_PATH "/" "libGTASA.so"
#define SHADER_CACHE_PATH  DATA_PATH "/" "cache"

#define SCREEN_W 960
#define SCREEN_H 544

#define TOUCH_X_MARGIN 100

#define ENABLE_SHADER_CACHE

#define FIX_SKIN_WEIGHTS
#define FIX_MAP_BOTTLENECK

#define ENABLE_SKYGFX
#define ENABLE_COLOR_FILTER_PS2
#define ENABLE_SUN_CORONA_PS2
#define DISABLE_DETAIL_TEXTURES
#define DISABLE_PED_SPEC
// #define DISABLE_ALPHA_TESTING
#define SLOW_GPU

#endif

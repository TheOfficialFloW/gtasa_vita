#ifndef __CONFIG_H__
#define __CONFIG_H__

// #define DEBUG

// #define LOAD_ADDRESS 0x98000000

#define MEMORY_SCELIBC_MB 8
#define MEMORY_NEWLIB_MB 232

#define DATA_PATH "ux0:data/gtasa"
#define SO_PATH DATA_PATH "/" "libGTASA.so"
#define SHADER_CACHE_PATH DATA_PATH "/" "cache"
#define CONFIG_PATH DATA_PATH "/" "config.txt"

#define SCREEN_W 960
#define SCREEN_H 544

enum SkyGfxColorFilter {
  SKYGFX_COLOR_FILTER_MOBILE,
  SKYGFX_COLOR_FILTER_NONE,
  SKYGFX_COLOR_FILTER_PS2,
  SKYGFX_COLOR_FILTER_PC,
};

typedef struct {
  int touch_x_margin;
  int enable_bones_optimization;
  int enable_mvp_optimization;
  int ignore_mobile_stuff;
  int fix_heli_plane_camera;
  int front_touch_triggers;
  int fix_skin_weights;
  int fix_map_bottleneck;
  int use_shader_cache;
  int skygfx_ps2_shading; // lighting and vehicle reflections
  int skygfx_colorfilter;
  int skygfx_ps2_sun;
  int disable_detail_textures;
  int disable_ped_spec;
  int disable_tex_bias;
  int disable_mipmaps;
  int aa_mode;
} Config;

extern Config config;

int read_config(const char *file);

#endif

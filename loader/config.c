/* config.c -- simple configuration parser
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include <vitasdk.h>

#include "main.h"
#include "config.h"

Config config;

int read_config(const char *file) {
  char name[64];
  int value;
  FILE *f;

  memset(&config, 0, sizeof(Config));
  config.touch_x_margin = 100;
  config.enable_bones_optimization = 0;
  config.enable_mvp_optimization = 0;
  config.ignore_mobile_stuff = 1;
  config.fix_heli_plane_camera = 1;
  config.fix_skin_weights = 1;
  config.fix_map_bottleneck = 1;
  config.use_shader_cache = 1;
  config.skygfx_ps2_shading = 1;
  config.skygfx_colorfilter = SKYGFX_COLOR_FILTER_PS2;
  config.skygfx_ps2_sun = 1;
  config.disable_detail_textures = 1;
  config.disable_ped_spec = 1;
  config.disable_tex_bias = 1;
  config.disable_mipmaps = 0;
  config.aa_mode = SCE_GXM_MULTISAMPLE_2X;

  f = fopen(file, "r");
  if (f == NULL)
    return -1;

  while ((fscanf(f, "%s %d", name, &value)) != EOF) {
    #define CONFIG_VAR(var) if (strcmp(name, #var) == 0) config.var = value;
    CONFIG_VAR(touch_x_margin);
    CONFIG_VAR(enable_bones_optimization);
    CONFIG_VAR(enable_mvp_optimization);
    CONFIG_VAR(ignore_mobile_stuff);
    CONFIG_VAR(fix_heli_plane_camera);
    CONFIG_VAR(fix_skin_weights);
    CONFIG_VAR(fix_map_bottleneck);
    CONFIG_VAR(use_shader_cache);
    CONFIG_VAR(skygfx_ps2_shading);
    CONFIG_VAR(skygfx_colorfilter);
    CONFIG_VAR(skygfx_ps2_sun);
    CONFIG_VAR(disable_detail_textures);
    CONFIG_VAR(disable_ped_spec);
    CONFIG_VAR(disable_tex_bias);
    CONFIG_VAR(disable_mipmaps);
    CONFIG_VAR(aa_mode);
    #undef CONFIG_VAR
  }

  fclose(f);

  return 0;
}

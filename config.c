/* config.c -- simple configuration parser
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "config.h"

Config config;

int read_config(const char *file) {
  char name[64];
  int value;
  FILE *f;

  memset(&config, 0, sizeof(Config));
  config.touch_x_margin = 100;
  config.use_fios2 = 1;
  config.io_cache_block_num = 64;
  config.io_cache_block_size = 65536;
  config.fix_heli_plane_camera = 1;
  config.fix_skin_weights = 1;
  config.fix_map_bottleneck = 1;
  config.enable_shader_cache = 1;
  config.enable_skygfx = 1;
  config.disable_detail_textures = 1;
  config.disable_ped_spec = 1;
  config.disable_tex_bias = 1;
  config.disable_alpha_testing = 0;

  f = fopen(file, "r");
  if (f == NULL)
    return -1;

  while ((fscanf(f, "%s %d", name, &value)) != EOF) {
    #define CONFIG_VAR(var) if (strcmp(name, #var) == 0) config.var = value;
    CONFIG_VAR(touch_x_margin);
    CONFIG_VAR(use_fios2);
    CONFIG_VAR(io_cache_block_num);
    CONFIG_VAR(io_cache_block_size);
    CONFIG_VAR(fix_heli_plane_camera);
    CONFIG_VAR(fix_skin_weights);
    CONFIG_VAR(fix_map_bottleneck);
    CONFIG_VAR(enable_shader_cache);
    CONFIG_VAR(enable_skygfx);
    CONFIG_VAR(disable_detail_textures);
    CONFIG_VAR(disable_ped_spec);
    CONFIG_VAR(disable_tex_bias);
    CONFIG_VAR(disable_alpha_testing);
    #undef CONFIG_VAR
  }

  fclose(f);

  return 0;
}

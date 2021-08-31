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

int mapping_count = 0;
ButtonMapping button_mapping[MAX_BUTTON_MAPPING];
Config config;

int read_config(const char *file) {
  memset(&config, 0, sizeof(Config));
  config.touch_x_margin = 100;
  config.front_touch_triggers = 0;
  config.fix_heli_plane_camera = 1;
  config.skygfx_colorfilter = SKYGFX_COLOR_FILTER_PS2;
  config.skygfx_ps2_shading = 1;
  config.skygfx_ps2_sun = 1;
  config.resolution = 0;
  config.aa_mode = SCE_GXM_MULTISAMPLE_2X;
  config.enable_high_detail_player = 0;
  config.disable_detail_textures = 1;
  config.disable_tex_bias = 1;
  config.disable_mipmaps = 0;
  config.fix_skin_weights = 1;
  config.disable_ped_spec = 1;
  config.ignore_mobile_stuff = 1;
  config.enable_fuzzy_seek = 0;
  config.use_shader_cache = 1;
  config.enable_mvp_optimization = 0;
  config.enable_bones_optimization = 0;

  FILE *f = fopen(file, "r");
  if (f == NULL)
    return -1;

  char line[1024];
  char name[64];
  int value;

  while (fgets(line, sizeof(line), f)) {
    if (line[0] == ';')
      continue;
    sscanf(line, "%s %d", name, &value);
    #define CONFIG_VAR(var) else if (strcmp(name, #var) == 0) config.var = value;
    if (0) {}
    CONFIG_VAR(touch_x_margin)
    CONFIG_VAR(front_touch_triggers)
    CONFIG_VAR(fix_heli_plane_camera)
    CONFIG_VAR(skygfx_colorfilter)
    CONFIG_VAR(skygfx_ps2_shading)
    CONFIG_VAR(skygfx_ps2_sun)
    CONFIG_VAR(resolution)
    CONFIG_VAR(aa_mode)
    CONFIG_VAR(enable_high_detail_player)
    CONFIG_VAR(disable_detail_textures)
    CONFIG_VAR(disable_tex_bias)
    CONFIG_VAR(disable_mipmaps)
    CONFIG_VAR(fix_skin_weights)
    CONFIG_VAR(disable_ped_spec)
    CONFIG_VAR(ignore_mobile_stuff)
    CONFIG_VAR(enable_fuzzy_seek)
    CONFIG_VAR(use_shader_cache)
    CONFIG_VAR(enable_mvp_optimization)
    CONFIG_VAR(enable_bones_optimization)
    #undef CONFIG_VAR
  }

  fclose(f);
  
  switch (config.resolution) {
  case 0:
    SCREEN_W = 960;
    SCREEN_H = 544;
    break;
  case 1:
    SCREEN_W = 1280;
    SCREEN_H = 725;
    break;
  case 2:
    SCREEN_W = 1920;
    SCREEN_H = 1088;
    break;
  }

  return 0;
}

typedef struct {
  char *name;
  ButtonID button_id;
} NameToButtonID;

static NameToButtonID name_to_button_ids[] = {
  { "BUTTON_UNUSED", BUTTON_UNUSED },
  { "BUTTON_CROSS", BUTTON_CROSS },
  { "BUTTON_CIRCLE", BUTTON_CIRCLE },
  { "BUTTON_SQUARE", BUTTON_SQUARE },
  { "BUTTON_TRIANGLE", BUTTON_TRIANGLE },
  { "BUTTON_L1", BUTTON_L1 },
  { "BUTTON_R1", BUTTON_R1 },
  { "BUTTON_L3", BUTTON_L3 },
  { "BUTTON_R3", BUTTON_R3 },
  { "BUTTON_L2", BUTTON_L2 },
  { "BUTTON_R2", BUTTON_R2 },
  { "BUTTON_SELECT", BUTTON_SELECT },
  { "BUTTON_START", BUTTON_START },
  { "DPAD_UP", DPAD_UP },
  { "DPAD_DOWN", DPAD_DOWN },
  { "DPAD_LEFT", DPAD_LEFT },
  { "DPAD_RIGHT", DPAD_RIGHT },
  { "ANALOG_LEFT_X", ANALOG_LEFT_X },
  { "ANALOG_LEFT_Y", ANALOG_LEFT_Y },
  { "ANALOG_RIGHT_X", ANALOG_RIGHT_X },
  { "ANALOG_RIGHT_Y", ANALOG_RIGHT_Y },
};

ButtonID GetButtonID(const char *name) {
  for (int i = 0; i < sizeof(name_to_button_ids) / sizeof(NameToButtonID); i++) {
    if (strcmp(name, name_to_button_ids[i].name) == 0) {
      return name_to_button_ids[i].button_id;
    }
  }

  return -1;
}

int read_controller_config(const char *file) {
  mapping_count = 0;

  FILE *f = fopen(file, "r");
  if (f == NULL)
    return -1;

  char line[1024];
  char name[64];
  char value[64];

  while (fgets(line, sizeof(line), f) && mapping_count < MAX_BUTTON_MAPPING) {
    if (line[0] == ';')
      continue;
    sscanf(line, "%s %s", name, value);
    #define CONFIG_VAR(var) else if (strcmp(name, #var) == 0) button_mapping[mapping_count++] = (ButtonMapping) { var, GetButtonID(value) };
    if (0) {}
    CONFIG_VAR(MAPPING_ATTACK)
    CONFIG_VAR(MAPPING_SPRINT)
    CONFIG_VAR(MAPPING_JUMP)
    CONFIG_VAR(MAPPING_CROUCH)
    CONFIG_VAR(MAPPING_ENTER_CAR)
    CONFIG_VAR(MAPPING_BRAKE)
    CONFIG_VAR(MAPPING_HANDBRAKE)
    CONFIG_VAR(MAPPING_ACCELERATE)
    CONFIG_VAR(MAPPING_CAMERA_CLOSER)
    CONFIG_VAR(MAPPING_CAMERA_FARTHER)
    CONFIG_VAR(MAPPING_HORN)
    CONFIG_VAR(MAPPING_RADIO_PREV_STATION)
    CONFIG_VAR(MAPPING_RADIO_NEXT_STATION)
    CONFIG_VAR(MAPPING_VITAL_STATS)
    CONFIG_VAR(MAPPING_NEXT_WEAPON)
    CONFIG_VAR(MAPPING_PREV_WEAPON)
    CONFIG_VAR(MAPPING_RADAR)
    CONFIG_VAR(MAPPING_PED_LOOK_BACK)
    CONFIG_VAR(MAPPING_VEHICLE_LOOK_LEFT)
    CONFIG_VAR(MAPPING_VEHICLE_LOOK_RIGHT)
    CONFIG_VAR(MAPPING_VEHICLE_LOOK_BACK)
    CONFIG_VAR(MAPPING_MISSION_START_AND_CANCEL)
    CONFIG_VAR(MAPPING_MISSION_START_AND_CANCEL_VIGILANTE)
    CONFIG_VAR(MAPPING_VEHICLE_STEER_X)
    CONFIG_VAR(MAPPING_VEHICLE_STEER_Y)
    CONFIG_VAR(MAPPING_VEHICLE_STEER_LEFT)
    CONFIG_VAR(MAPPING_VEHICLE_STEER_RIGHT)
    CONFIG_VAR(MAPPING_LOOK_X)
    CONFIG_VAR(MAPPING_LOOK_Y)
    CONFIG_VAR(MAPPING_PED_MOVE_X)
    CONFIG_VAR(MAPPING_PED_MOVE_Y)
    CONFIG_VAR(MAPPING_AUTO_HYDRAULICS)
    CONFIG_VAR(MAPPING_SWAP_WEAPONS_AND_PURCHASE)
    CONFIG_VAR(MAPPING_WEAPON_ZOOM_IN)
    CONFIG_VAR(MAPPING_WEAPON_ZOOM_OUT)
    CONFIG_VAR(MAPPING_ENTER_AND_EXIT_TARGETING)
    CONFIG_VAR(MAPPING_VEHICLE_BOMB)
    CONFIG_VAR(MAPPING_TURRET_LEFT)
    CONFIG_VAR(MAPPING_TURRET_RIGHT)
    CONFIG_VAR(MAPPING_MAGNET)
    CONFIG_VAR(MAPPING_SKIP_CUTSCENE)
    CONFIG_VAR(MAPPING_GANG_RECRUIT)
    CONFIG_VAR(MAPPING_GANG_IGNORE)
    CONFIG_VAR(MAPPING_GANG_FOLLOW)
    CONFIG_VAR(MAPPING_GANG_HOLD_POSITION)
    CONFIG_VAR(MAPPING_RHYTHM_UP)
    CONFIG_VAR(MAPPING_RHYTHM_DOWN)
    CONFIG_VAR(MAPPING_RHYTHM_LEFT)
    CONFIG_VAR(MAPPING_RHYTHM_RIGHT)
    CONFIG_VAR(MAPPING_DROP_CRANE)
    CONFIG_VAR(MAPPING_DROP_ITEM)
    CONFIG_VAR(MAPPING_PHONE)
    CONFIG_VAR(MAPPING_NITRO)
    CONFIG_VAR(MAPPING_CRANE_UP)
    CONFIG_VAR(MAPPING_CRANE_DOWN)
    CONFIG_VAR(MAPPING_ACCEPT)
    CONFIG_VAR(MAPPING_CANCEL)
    CONFIG_VAR(MAPPING_GRAB)
    CONFIG_VAR(MAPPING_STINGER)
    CONFIG_VAR(MAPPING_MENU_DOWN)
    CONFIG_VAR(MAPPING_MENU_UP)
    CONFIG_VAR(MAPPING_MENU_LEFT)
    CONFIG_VAR(MAPPING_MENU_RIGHT)
    CONFIG_VAR(MAPPING_MENU_ACCEPT)
    CONFIG_VAR(MAPPING_MENU_BACK)
    CONFIG_VAR(MAPPING_MENU_MAP)
    CONFIG_VAR(MAPPING_ARCADE_BUTTON)
    CONFIG_VAR(MAPPING_ARCADE_POWER_OFF)
    CONFIG_VAR(MAPPING_ARCADE_RESET)
    CONFIG_VAR(MAPPING_ARCADE_JOYSTICK)
    CONFIG_VAR(MAPPING_GYM_ACTION)
    CONFIG_VAR(MAPPING_GYM_EASIER_LEVEL)
    CONFIG_VAR(MAPPING_GYM_HARDER_LEVEL)
    CONFIG_VAR(MAPPING_BLACK_JACK_SPLIT)
    CONFIG_VAR(MAPPING_BLACK_JACK_DOUBLE)
    CONFIG_VAR(MAPPING_BLACK_JACK_HIT)
    CONFIG_VAR(MAPPING_BLACK_JACK_STAND)
    CONFIG_VAR(MAPPING_PLACE_BET)
    CONFIG_VAR(MAPPING_REMOVE_BET)
    CONFIG_VAR(MAPPING_NEXT_TARGET)
    CONFIG_VAR(MAPPING_PREV_TARGET)
    CONFIG_VAR(MAPPING_WAYPOINT_BLIP)
    CONFIG_VAR(MAPPING_HELICOPTER_MAGNET_UP)
    CONFIG_VAR(MAPPING_HELICOPTER_MAGNET_DOWN)
    CONFIG_VAR(MAPPING_LOCK_HYDRAULICS)
    CONFIG_VAR(MAPPING_FLIGHT_ASCEND)
    CONFIG_VAR(MAPPING_FLIGHT_DESCEND)
    CONFIG_VAR(MAPPING_FLIGHT_PRIMARY_ATTACK)
    CONFIG_VAR(MAPPING_FLIGHT_SECONDARY_ATTACK)
    CONFIG_VAR(MAPPING_FLIGHT_ALT_LEFT)
    CONFIG_VAR(MAPPING_FLIGHT_ALT_RIGHT)
    CONFIG_VAR(MAPPING_FLIGHT_ALT_UP)
    CONFIG_VAR(MAPPING_FLIGHT_ALT_DOWN)
    CONFIG_VAR(MAPPING_BASKETBALL_SHOOT)
    CONFIG_VAR(MAPPING_BUNNY_HOP)
    CONFIG_VAR(MAPPING_MAP_ZOOM_IN)
    CONFIG_VAR(MAPPING_MAP_ZOOM_OUT)
    CONFIG_VAR(MAPPING_ALT_ATTACK)
    CONFIG_VAR(MAPPING_BLOCK)
    CONFIG_VAR(MAPPING_TAKE_COVER_LEFT)
    CONFIG_VAR(MAPPING_TAKE_COVER_RIGHT)
    CONFIG_VAR(MAPPING_TOGGLE_LANDING_GEAR)
    CONFIG_VAR(MAPPING_KISS)
    CONFIG_VAR(MAPPING_DANCING_UP)
    CONFIG_VAR(MAPPING_DANCING_DOWN)
    CONFIG_VAR(MAPPING_DANCING_LEFT)
    CONFIG_VAR(MAPPING_DANCING_RIGHT)
    CONFIG_VAR(MAPPING_REPLAY)
    #undef CONFIG_VAR
  }

  fclose(f);

  return 0;
}


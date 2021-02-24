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

  FILE *f = fopen(file, "r");
  if (f == NULL)
    return -1;

  char name[64];
  int value;

  while ((fscanf(f, "%s %d", name, &value)) != EOF) {
    if (name[0] == ';')
      continue;

    #define CONFIG_VAR(var) else if (strcmp(name, #var) == 0) config.var = value;
    if (0) {}
    CONFIG_VAR(touch_x_margin)
    CONFIG_VAR(front_touch_triggers)
    CONFIG_VAR(enable_bones_optimization)
    CONFIG_VAR(enable_mvp_optimization)
    CONFIG_VAR(ignore_mobile_stuff)
    CONFIG_VAR(fix_heli_plane_camera)
    CONFIG_VAR(fix_skin_weights)
    CONFIG_VAR(fix_map_bottleneck)
    CONFIG_VAR(use_shader_cache)
    CONFIG_VAR(skygfx_ps2_shading)
    CONFIG_VAR(skygfx_colorfilter)
    CONFIG_VAR(skygfx_ps2_sun)
    CONFIG_VAR(disable_detail_textures)
    CONFIG_VAR(disable_ped_spec)
    CONFIG_VAR(disable_tex_bias)
    CONFIG_VAR(disable_mipmaps)
    CONFIG_VAR(aa_mode)
    #undef CONFIG_VAR
  }

  fclose(f);

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

void set_default_mapping(void) {
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ATTACK, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_SPRINT, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_JUMP, BUTTON_SQUARE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_CROUCH, BUTTON_L3 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ENTER_CAR, BUTTON_TRIANGLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_BRAKE, BUTTON_SQUARE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_HANDBRAKE, BUTTON_R1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ACCELERATE, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_CAMERA_CLOSER, BUTTON_SELECT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_CAMERA_FARTHER, BUTTON_SELECT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_HORN, BUTTON_L3 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_RADIO_PREV_STATION, DPAD_UP };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_RADIO_NEXT_STATION, DPAD_DOWN };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_VITAL_STATS, DPAD_LEFT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_NEXT_WEAPON, BUTTON_R2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_PREV_WEAPON, BUTTON_L2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_RADAR, BUTTON_START };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_PED_LOOK_BACK, BUTTON_R3 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_VEHICLE_LOOK_LEFT, BUTTON_L2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_VEHICLE_LOOK_RIGHT, BUTTON_R2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MISSION_START_AND_CANCEL, BUTTON_TRIANGLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MISSION_START_AND_CANCEL_VIGILANTE, BUTTON_R3 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_VEHICLE_STEER_X, ANALOG_LEFT_X };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_VEHICLE_STEER_Y, ANALOG_LEFT_Y };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_LOOK_X, ANALOG_RIGHT_X };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_LOOK_Y, ANALOG_RIGHT_Y };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_PED_MOVE_X, ANALOG_LEFT_X };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_PED_MOVE_Y, ANALOG_LEFT_Y };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_AUTO_HYDRAULICS, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_SWAP_WEAPONS_AND_PURCHASE, BUTTON_L1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_WEAPON_ZOOM_IN, BUTTON_L2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_WEAPON_ZOOM_OUT, BUTTON_R2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ENTER_AND_EXIT_TARGETING, BUTTON_R1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_VEHICLE_BOMB, DPAD_LEFT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_TURRET_LEFT, BUTTON_L2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_TURRET_RIGHT, BUTTON_R2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MAGNET, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_SKIP_CUTSCENE, BUTTON_START };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_SKIP_CUTSCENE, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GANG_RECRUIT, DPAD_LEFT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GANG_IGNORE, DPAD_RIGHT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GANG_FOLLOW, DPAD_UP };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GANG_HOLD_POSITION, DPAD_DOWN };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_RHYTHM_UP, ANALOG_LEFT_Y };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_RHYTHM_DOWN, ANALOG_LEFT_Y };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_RHYTHM_LEFT, ANALOG_LEFT_X };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_RHYTHM_RIGHT, ANALOG_LEFT_X };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_DROP_CRANE, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_DROP_ITEM, BUTTON_TRIANGLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_PHONE, BUTTON_L1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_NITRO, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_CRANE_UP, BUTTON_SQUARE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_CRANE_DOWN, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ACCEPT, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_CANCEL, BUTTON_TRIANGLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GRAB, BUTTON_L1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_STINGER, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_DOWN, DPAD_DOWN };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_UP, DPAD_UP };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_LEFT, DPAD_LEFT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_RIGHT, DPAD_RIGHT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_ACCEPT, BUTTON_START };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_ACCEPT, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_BACK, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MENU_MAP, BUTTON_SELECT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ARCADE_BUTTON, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ARCADE_POWER_OFF, BUTTON_TRIANGLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ARCADE_RESET, BUTTON_SELECT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_ARCADE_JOYSTICK, ANALOG_LEFT_X };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GYM_ACTION, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GYM_EASIER_LEVEL, DPAD_LEFT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_GYM_HARDER_LEVEL, DPAD_RIGHT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_BLACK_JACK_SPLIT, BUTTON_R1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_BLACK_JACK_DOUBLE, BUTTON_SQUARE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_BLACK_JACK_HIT, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_BLACK_JACK_STAND, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_PLACE_BET, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_REMOVE_BET, BUTTON_SQUARE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_NEXT_TARGET, BUTTON_R2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_PREV_TARGET, BUTTON_L2 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_WAYPOINT_BLIP, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_HELICOPTER_MAGNET_UP, DPAD_LEFT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_HELICOPTER_MAGNET_DOWN, DPAD_RIGHT };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_LOCK_HYDRAULICS, BUTTON_R3 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_FLIGHT_PRIMARY_ATTACK, BUTTON_R1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_FLIGHT_SECONDARY_ATTACK, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_BASKETBALL_SHOOT, BUTTON_CIRCLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_BUNNY_HOP, BUTTON_L1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MAP_ZOOM_IN, BUTTON_L1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_MAP_ZOOM_OUT, BUTTON_R1 };
  button_mapping[mapping_count++] = (ButtonMapping) { HID_MAPPING_ALT_ATTACK, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { HID_MAPPING_BLOCK, BUTTON_SQUARE };
  button_mapping[mapping_count++] = (ButtonMapping) { HID_MAPPING_TAKE_COVER_LEFT, BUTTON_L2 };
  button_mapping[mapping_count++] = (ButtonMapping) { HID_MAPPING_TAKE_COVER_RIGHT, BUTTON_R2 };
  button_mapping[mapping_count++] = (ButtonMapping) { HID_MAPPING_TOGGLE_LANDING_GEAR, BUTTON_L3 };
  button_mapping[mapping_count++] = (ButtonMapping) { HID_MAPPING_KISS, BUTTON_L1 };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_DANCING_UP, BUTTON_TRIANGLE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_DANCING_DOWN, BUTTON_CROSS };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_DANCING_LEFT, BUTTON_SQUARE };
  button_mapping[mapping_count++] = (ButtonMapping) { MAPPING_DANCING_RIGHT, BUTTON_CIRCLE };
}

int read_controller_config(const char *file) {
  mapping_count = 0;

  FILE *f = fopen(file, "r");
  if (f == NULL) {
    set_default_mapping();
    return -1;
  }

  char name[64];
  char value[64];

  while ((fscanf(f, "%s %s", name, value)) != EOF && mapping_count < MAX_BUTTON_MAPPING) {
    if (name[0] == ';')
      continue;

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
    CONFIG_VAR(HID_MAPPING_ALT_ATTACK)
    CONFIG_VAR(HID_MAPPING_BLOCK)
    CONFIG_VAR(HID_MAPPING_TAKE_COVER_LEFT)
    CONFIG_VAR(HID_MAPPING_TAKE_COVER_RIGHT)
    CONFIG_VAR(HID_MAPPING_TOGGLE_LANDING_GEAR)
    CONFIG_VAR(HID_MAPPING_KISS)
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


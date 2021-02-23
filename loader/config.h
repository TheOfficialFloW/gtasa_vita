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
#define CONTROLLER_CONFIG_PATH DATA_PATH "/" "controls.txt"

#define SCREEN_W 960
#define SCREEN_H 544

enum SkyGfxColorFilter {
  SKYGFX_COLOR_FILTER_MOBILE,
  SKYGFX_COLOR_FILTER_NONE,
  SKYGFX_COLOR_FILTER_PS2,
  SKYGFX_COLOR_FILTER_PC,
};
enum Mappings{
	MAPPING_UNKNOWN = 0,
	MAPPING_ATTACK,
	MAPPING_SPRINT,
	MAPPING_JUMP,
	MAPPING_CROUCH,
	MAPPING_ENTER_CAR,
	MAPPING_BRAKE,
	MAPPING_HANDBRAKE,
	MAPPING_ACCELERATE,
	MAPPING_CAMERA_CLOSER,
	MAPPING_CAMERA_FARTHER,
	MAPPING_HORN,
	MAPPING_RADIO_PREV_STATION,
	MAPPING_RADIO_NEXT_STATION,
	MAPPING_VITAL_STATS,
	MAPPING_NEXT_WEAPON,
	MAPPING_PREV_WEAPON,
	MAPPING_RADAR,
	MAPPING_PED_LOOK_BACK,
	MAPPING_VEHICLE_LOOK_LEFT,
	MAPPING_VEHICLE_LOOK_RIGHT,
	MAPPING_VEHICLE_LOOK_BACK,
	MAPPING_MISSION_START_AND_CANCEL,
	MAPPING_MISSION_START_AND_CANCEL_VIGILANTE,
	MAPPING_VEHICLE_STEER_X,
	MAPPING_VEHICLE_STEER_Y,
	MAPPING_VEHICLE_STEER_LEFT,
	MAPPING_VEHICLE_STEER_RIGHT,
	MAPPING_LOOK_X,
	MAPPING_LOOK_Y,
	MAPPING_PED_MOVE_X,
	MAPPING_PED_MOVE_Y,
	MAPPING_AUTO_HYDRAULICS,
	MAPPING_SWAP_WEAPONS_AND_PURCHASE,
	MAPPING_WEAPON_ZOOM_IN,
	MAPPING_WEAPON_ZOOM_OUT,
	MAPPING_ENTER_AND_EXIT_TARGETING,
	MAPPING_VEHICLE_BOMB,
	MAPPING_TURRET_LEFT,
	MAPPING_TURRET_RIGHT,
	MAPPING_MAGNET,
	MAPPING_SKIP_CUTSCENE,
	MAPPING_GANG_RECRUIT,
	MAPPING_GANG_IGNORE,
	MAPPING_GANG_FOLLOW,
	MAPPING_GANG_HOLD_POSITION,
	MAPPING_RHYTHM_UP,
	MAPPING_RHYTHM_DOWN,
	MAPPING_RHYTHM_LEFT,
	MAPPING_RHYTHM_RIGHT,
	MAPPING_DROP_CRANE,
	MAPPING_DROP_ITEM,
	MAPPING_PHONE,
	MAPPING_NITRO,
	MAPPING_CRANE_UP,
	MAPPING_CRANE_DOWN,
	MAPPING_ACCEPT,
	MAPPING_CANCEL,
	MAPPING_GRAB,
	MAPPING_STINGER,
	MAPPING_MENU_DOWN,
	MAPPING_MENU_UP,
	MAPPING_MENU_LEFT,
	MAPPING_MENU_RIGHT,
	MAPPING_MENU_ACCEPT,
	MAPPING_MENU_BACK,
	MAPPING_MENU_MAP,
	MAPPING_ARCADE_BUTTON,
	MAPPING_ARCADE_POWER_OFF,
	MAPPING_ARCADE_RESET,
	MAPPING_ARCADE_JOYSTICK,
	MAPPING_GYM_ACTION,
	MAPPING_GYM_EASIER_LEVEL,
	MAPPING_GYM_HARDER_LEVEL,
	MAPPING_BLACK_JACK_SPLIT,
	MAPPING_BLACK_JACK_DOUBLE,
	MAPPING_BLACK_JACK_HIT,
	MAPPING_BLACK_JACK_STAND,
	MAPPING_PLACE_BET,
	MAPPING_REMOVE_BET,
	MAPPING_NEXT_TARGET,
	MAPPING_PREV_TARGET,
	MAPPING_WAYPOINT_BLIP,
	MAPPING_HELICOPTER_MAGNET_UP,
	MAPPING_HELICOPTER_MAGNET_DOWN,
	MAPPING_LOCK_HYDRAULICS,
	MAPPING_FLIGHT_ASCEND,
	MAPPING_FLIGHT_DESCEND,
	MAPPING_FLIGHT_PRIMARY_ATTACK,
	MAPPING_FLIGHT_SECONDARY_ATTACK,
	MAPPING_FLIGHT_ALT_LEFT,
	MAPPING_FLIGHT_ALT_RIGHT,
	MAPPING_FLIGHT_ALT_UP,
	MAPPING_FLIGHT_ALT_DOWN,
	MAPPING_BASKETBALL_SHOOT,
	MAPPING_BUNNY_HOP,
	MAPPING_MAP_ZOOM_IN,
	MAPPING_MAP_ZOOM_OUTHID_,
	MAPPING_ALT_ATTACKHID_,
	MAPPING_BLOCKHID_,
	MAPPING_TAKE_COVER_LEFTHID_,
	MAPPING_TAKE_COVER_RIGHTHID_,
	MAPPING_TOGGLE_LANDING_GEARHID_,
	MAPPING_KISS,
	MAPPING_DANCING_UP,
	MAPPING_DANCING_DOWN,
	MAPPING_DANCING_LEFT,
	MAPPING_DANCING_RIGHT,
	MAPPING_REPLAY,
};

enum ButtonID{
	BUTTON_CROSS = 0,
	BUTTON_CIRCLE = 1,
	BUTTON_SQAURE = 2,
	BUTTON_TRINAGLE = 3,
	BUTTON_START = 4,
	BUTTON_SELECT = 5,
	BUTTON_L1 = 6,
	BUTTON_R1 = 7,
	DPAD_UP = 8,
	DPAD_DOWN = 9,
	DPAD_LEFT = 10,
	DPAD_RIGHT = 11,
	BUTTON_L3 = 12,
	BUTTON_R3 = 13,
	ANALOG_LEFT_X = 64,
	ANALOG_LEFT_Y =65,
	ANALOG_RIGHT_X = 66,
	ANALOG_RIGHT_Y = 67,
	BUTTON_L2 = 68,
	BUTTON_R2 = 69,
};



typedef struct {
  int touch_x_margin;
  int front_touch_triggers;
  int enable_bones_optimization;
  int enable_mvp_optimization;
  int ignore_mobile_stuff;
  int fix_heli_plane_camera;
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
  int custom_controls;
} Config;

extern Config config;

int read_config(const char *file);
int read_controller_config(const char *file);

#endif

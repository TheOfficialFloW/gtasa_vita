#include <vitasdk.h>
#include <vitaGL.h>
#include <imgui_vita.h>
#include <stdio.h>
#include <string>

extern "C" {
void *__wrap_memcpy(void *dest, const void *src, size_t n) {
  return sceClibMemcpy(dest, src, n);
}

void *__wrap_memmove(void *dest, const void *src, size_t n) {
  return sceClibMemmove(dest, src, n);
}

void *__wrap_memset(void *s, int c, size_t n) {
  return sceClibMemset(s, c, n);
}
}

#define CONFIG_FILE_PATH "ux0:data/gtasa/config.txt"
#define CONTROLS_FILE_PATH "ux0:data/gtasa/controls.txt"

#define SKYGFX_COLOR_FILTER_NUM 4
enum SkyGfxColorFilter {
  SKYGFX_COLOR_FILTER_MOBILE,
  SKYGFX_COLOR_FILTER_NONE,
  SKYGFX_COLOR_FILTER_PS2,
  SKYGFX_COLOR_FILTER_PC,
};

char *SkyGfxColorFilterName[SKYGFX_COLOR_FILTER_NUM] = {
  "Mobile",
  "None",
  "PS2",
  "PC"
};

#define ANTI_ALIASING_NUM 3
char *AntiAliasingName[ANTI_ALIASING_NUM] = {
  "Disabled",
  "MSAA 2x",
  "MSAA 4x"
};

#define CONTROLS_NUM 21
char *ControlsName[CONTROLS_NUM] = {
  "Unused",
  "Cross",
  "Circle",
  "Square",
  "Triangle",
  "L1",
  "R1",
  "L3",
  "R3",
  "L2",
  "R2",
  "Select",
  "Start",
  "Up",
  "Down",
  "Left",
  "Right",
  "Analog Left (X Axis)",
  "Analog Left (Y Axis)",
  "Analog Right (X Axis)",
  "Analog Right (Y Axis)"
};

char *ControlsVar[CONTROLS_NUM] = {
  "BUTTON_UNUSED",
  "BUTTON_CROSS",
  "BUTTON_CIRCLE",
  "BUTTON_SQUARE",
  "BUTTON_TRIANGLE",
  "BUTTON_L1",
  "BUTTON_R1",
  "BUTTON_L3",
  "BUTTON_R3",
  "BUTTON_L2",
  "BUTTON_R2",
  "BUTTON_SELECT",
  "BUTTON_START",
  "DPAD_UP",
  "DPAD_DOWN",
  "DPAD_LEFT",
  "DPAD_RIGHT",
  "ANALOG_LEFT_X",
  "ANALOG_LEFT_Y",
  "ANALOG_RIGHT_X",
  "ANALOG_RIGHT_Y"
};

#define CONTROLS_MAPPINGS_NUM 108
char *ControlsMapName[CONTROLS_MAPPINGS_NUM] = {
  "Attack:",
  "Sprint:",
  "Jump:",
  "Crouch:",
  "Enter Car:",
  "Brake:",
  "Handbrake:",
  "Accelerate:",
  "Camera Zoom In:",
  "Camera Zoom Out:",
  "Horn:",
  "Previous Station (Radio):",
  "Next Station (Radio):",
  "Show Vital Stats:",
  "Next Weapon:",
  "Previous Weapon:",
  "Radar:",
  "Look Back (Walk):",
  "Look Left (Vehicle):",
  "Look Right (Vehicle):",
  "Look Back (Vehicle):",
  "Mission Start/Cancel:",
  "Vigilante Start/Cancel:",
  "Steer X:",
  "Steer Y:",
  "Steer Left:",
  "Steer Right:",
  "Look X:",
  "Look Y:",
  "Move X:",
  "Move Y:",
  "Auto Hydraulics:",
  "Swap Weapon and Purchase:",
  "Weapon Zoom In:",
  "Weapon Zoom Out:",
  "Enter/Exit Targeting:",
  "Vehicle Bomb:",
  "Turret Left:",
  "Turret Right:",
  "Magnet:",
  "Skip Cutscene:",
  "Gang Recruit:",
  "Gang Ignore:",
  "Gang Follow:",
  "Gang Hold Position:",
  "Rythm Up:",
  "Rythm Down:",
  "Rythm Left:",
  "Rythm Right:",
  "Drop Crane:",
  "Drop Item:",
  "Phone:",
  "Nitro:",
  "Crane Up:",
  "Crane Down:",
  "Accept:",
  "Cancel:",
  "Grab:",
  "Stinger:",
  "Menu Down:",
  "Menu Up:",
  "Menu Left:",
  "Menu Right:",
  "Menu Accept:",
  "Menu Back:",
  "Menu Map:",
  "Arcade Button:",
  "Arcade Power Off:",
  "Arcade Reset:",
  "Arcade Joystick:",
  "Gym Action:",
  "Gym Easier Level:",
  "Gym Harder Level:",
  "Blackjack Split:",
  "Blackjack Double:",
  "Blackjack Hit:",
  "Blackjack Stand:",
  "Place Bet:",
  "Remove Bet:",
  "Next Target:",
  "Previous Target:",
  "Waypoint Blip:",
  "Helicopter Magnet Up:",
  "Helicopter Magnet Down:",
  "Lock Hydraulics:",
  "Flight Ascend:",
  "Flight Descend:",
  "Flight Primary Attack:",
  "Flight Secondary Attack:",
  "Flight Left:",
  "Flight Right:",
  "Flight Up:",
  "Flight Down:",
  "Basketball Shoot:",
  "Bunny Hop:",
  "Map Zoom In:",
  "Map Zoom Out:",
  "Alt Attack:",
  "Block:",
  "Take Cover Left:",
  "Take Cover Right:",
  "Toggle Landing Gear:",
  "Kiss:",
  "Dancing Up:",
  "Dancing Down:",
  "Dancing Left:",
  "Dancing Right:",
  "Replay"
};

char *ControlsMapVar[CONTROLS_MAPPINGS_NUM] = {
  "MAPPING_ATTACK",
  "MAPPING_SPRINT",
  "MAPPING_JUMP",
  "MAPPING_CROUCH",
  "MAPPING_ENTER_CAR",
  "MAPPING_BRAKE",
  "MAPPING_HANDBRAKE",
  "MAPPING_ACCELERATE",
  "MAPPING_CAMERA_CLOSER",
  "MAPPING_CAMERA_FARTHER",
  "MAPPING_HORN",
  "MAPPING_RADIO_PREV_STATION",
  "MAPPING_RADIO_NEXT_STATION",
  "MAPPING_VITAL_STATS",
  "MAPPING_NEXT_WEAPON",
  "MAPPING_PREV_WEAPON",
  "MAPPING_RADAR",
  "MAPPING_PED_LOOK_BACK",
  "MAPPING_VEHICLE_LOOK_LEFT",
  "MAPPING_VEHICLE_LOOK_RIGHT",
  "MAPPING_VEHICLE_LOOK_BACK",
  "MAPPING_MISSION_START_AND_CANCEL",
  "MAPPING_MISSION_START_AND_CANCEL_VIGILANTE",
  "MAPPING_VEHICLE_STEER_X",
  "MAPPING_VEHICLE_STEER_Y",
  "MAPPING_VEHICLE_STEER_LEFT",
  "MAPPING_VEHICLE_STEER_RIGHT",
  "MAPPING_LOOK_X",
  "MAPPING_LOOK_Y",
  "MAPPING_PED_MOVE_X",
  "MAPPING_PED_MOVE_Y",
  "MAPPING_AUTO_HYDRAULICS",
  "MAPPING_SWAP_WEAPONS_AND_PURCHASE",
  "MAPPING_WEAPON_ZOOM_IN",
  "MAPPING_WEAPON_ZOOM_OUT",
  "MAPPING_ENTER_AND_EXIT_TARGETING",
  "MAPPING_VEHICLE_BOMB",
  "MAPPING_TURRET_LEFT",
  "MAPPING_TURRET_RIGHT",
  "MAPPING_MAGNET",
  "MAPPING_SKIP_CUTSCENE",
  "MAPPING_GANG_RECRUIT",
  "MAPPING_GANG_IGNORE",
  "MAPPING_GANG_FOLLOW",
  "MAPPING_GANG_HOLD_POSITION",
  "MAPPING_RHYTHM_UP",
  "MAPPING_RHYTHM_DOWN",
  "MAPPING_RHYTHM_LEFT",
  "MAPPING_RHYTHM_RIGHT",
  "MAPPING_DROP_CRANE",
  "MAPPING_DROP_ITEM",
  "MAPPING_PHONE",
  "MAPPING_NITRO",
  "MAPPING_CRANE_UP",
  "MAPPING_CRANE_DOWN",
  "MAPPING_ACCEPT",
  "MAPPING_CANCEL",
  "MAPPING_GRAB",
  "MAPPING_STINGER",
  "MAPPING_MENU_DOWN",
  "MAPPING_MENU_UP",
  "MAPPING_MENU_LEFT",
  "MAPPING_MENU_RIGHT",
  "MAPPING_MENU_ACCEPT",
  "MAPPING_MENU_BACK",
  "MAPPING_MENU_MAP",
  "MAPPING_ARCADE_BUTTON",
  "MAPPING_ARCADE_POWER_OFF",
  "MAPPING_ARCADE_RESET",
  "MAPPING_ARCADE_JOYSTICK",
  "MAPPING_GYM_ACTION",
  "MAPPING_GYM_EASIER_LEVEL",
  "MAPPING_GYM_HARDER_LEVEL",
  "MAPPING_BLACK_JACK_SPLIT",
  "MAPPING_BLACK_JACK_DOUBLE",
  "MAPPING_BLACK_JACK_HIT",
  "MAPPING_BLACK_JACK_STAND",
  "MAPPING_PLACE_BET",
  "MAPPING_REMOVE_BET",
  "MAPPING_NEXT_TARGET",
  "MAPPING_PREV_TARGET",
  "MAPPING_WAYPOINT_BLIP",
  "MAPPING_HELICOPTER_MAGNET_UP",
  "MAPPING_HELICOPTER_MAGNET_DOWN",
  "MAPPING_LOCK_HYDRAULICS",
  "MAPPING_FLIGHT_ASCEND",
  "MAPPING_FLIGHT_DESCEND",
  "MAPPING_FLIGHT_PRIMARY_ATTACK",
  "MAPPING_FLIGHT_SECONDARY_ATTACK",
  "MAPPING_FLIGHT_ALT_LEFT",
  "MAPPING_FLIGHT_ALT_RIGHT",
  "MAPPING_FLIGHT_ALT_UP",
  "MAPPING_FLIGHT_ALT_DOWN",
  "MAPPING_BASKETBALL_SHOOT",
  "MAPPING_BUNNY_HOP",
  "MAPPING_MAP_ZOOM_IN",
  "MAPPING_MAP_ZOOM_OUT",
  "MAPPING_ALT_ATTACK",
  "MAPPING_BLOCK",
  "MAPPING_TAKE_COVER_LEFT",
  "MAPPING_TAKE_COVER_RIGHT",
  "MAPPING_TOGGLE_LANDING_GEAR",
  "MAPPING_KISS",
  "MAPPING_DANCING_UP",
  "MAPPING_DANCING_DOWN",
  "MAPPING_DANCING_LEFT",
  "MAPPING_DANCING_RIGHT",
  "MAPPING_REPLAY"
};

int controls_map[CONTROLS_MAPPINGS_NUM];
int secondary_controls_map[CONTROLS_MAPPINGS_NUM];
int backup_controls_map[CONTROLS_MAPPINGS_NUM];
int backup_secondary_controls_map[CONTROLS_MAPPINGS_NUM];

int touch_x_margin = 100;
bool front_touch_triggers = false;
bool fix_heli_plane_camera = true;

int skygfx_colorfilter = SKYGFX_COLOR_FILTER_PS2;
bool skygfx_ps2_shading = true;
bool skygfx_ps2_sun = true;

int aa_mode = SCE_GXM_MULTISAMPLE_2X;
bool high_detail_player = false;
bool detail_textures = false;
bool tex_bias = false;
bool mipmaps = true;
bool fix_skin_weights = true;
bool ped_spec = false;
bool mobile_stuff = false;

bool allow_removed_tracks = false;
bool fuzzy_seek = false;
bool use_shader_cache = true;
bool enable_mvp_optimization = false;
bool enable_bones_optimization = false;

bool show_controls_window = false;

void loadConfig(void) {
  char buffer[30];
  int value;

  FILE *config = fopen(CONFIG_FILE_PATH, "r");

  if (config) {
    while (EOF != fscanf(config, "%[^ ] %d\n", buffer, &value)) {
      if (strcmp("touch_x_margin", buffer) == 0) touch_x_margin = value;
      else if (strcmp("front_touch_triggers", buffer) == 0) front_touch_triggers = (bool)value;
      else if (strcmp("fix_heli_plane_camera", buffer) == 0) fix_heli_plane_camera = (bool)value;

      else if (strcmp("skygfx_colorfilter", buffer) == 0) skygfx_colorfilter = value;
      else if (strcmp("skygfx_ps2_shading", buffer) == 0) skygfx_ps2_shading = (bool)value;
      else if (strcmp("skygfx_ps2_sun", buffer) == 0) skygfx_ps2_sun = (bool)value;

      else if (strcmp("aa_mode", buffer) == 0) aa_mode = value;
      else if (strcmp("enable_high_detail_player", buffer) == 0) high_detail_player = (bool)value;
      else if (strcmp("disable_detail_textures", buffer) == 0) detail_textures = value ? false : true;
      else if (strcmp("disable_tex_bias", buffer) == 0) tex_bias = value ? false : true;
      else if (strcmp("disable_mipmaps", buffer) == 0) mipmaps = value ? false : true;
      else if (strcmp("fix_skin_weights", buffer) == 0) fix_skin_weights = (bool)value;
      else if (strcmp("disable_ped_spec", buffer) == 0) ped_spec = value ? false : true;
      else if (strcmp("ignore_mobile_stuff", buffer) == 0) mobile_stuff = value ? false : true;

      else if (strcmp("allow_removed_tracks", buffer) == 0) allow_removed_tracks = (bool)value;

      else if (strcmp("enable_fuzzy_seek", buffer) == 0) fuzzy_seek = (bool)value;
      else if (strcmp("use_shader_cache", buffer) == 0) use_shader_cache = (bool)value;
      else if (strcmp("enable_mvp_optimization", buffer) == 0) enable_mvp_optimization = (bool)value;
      else if (strcmp("enable_bones_optimization", buffer) == 0) enable_bones_optimization = (bool)value;
    }
    fclose(config);
  }
}

void saveConfig(void) {
  FILE *config = fopen(CONFIG_FILE_PATH, "w+");

  if (config) {
    fprintf(config, "%s %d\n", "touch_x_margin", touch_x_margin);
    fprintf(config, "%s %d\n", "front_touch_triggers", (int)front_touch_triggers);
    fprintf(config, "%s %d\n", "fix_heli_plane_camera", (int)fix_heli_plane_camera);

    fprintf(config, "%s %d\n", "skygfx_colorfilter", skygfx_colorfilter);
    fprintf(config, "%s %d\n", "skygfx_ps2_shading", (int)skygfx_ps2_shading);
    fprintf(config, "%s %d\n", "skygfx_ps2_sun", (int)skygfx_ps2_sun);

    fprintf(config, "%s %d\n", "aa_mode", aa_mode);
    fprintf(config, "%s %d\n", "enable_high_detail_player", (int)high_detail_player);
    fprintf(config, "%s %d\n", "disable_detail_textures", detail_textures ? false : true);
    fprintf(config, "%s %d\n", "disable_tex_bias", tex_bias ? false : true);
    fprintf(config, "%s %d\n", "disable_mipmaps", mipmaps ? false : true);
    fprintf(config, "%s %d\n", "fix_skin_weights", (int)fix_skin_weights);
    fprintf(config, "%s %d\n", "disable_ped_spec", ped_spec ? false : true);
    fprintf(config, "%s %d\n", "ignore_mobile_stuff", mobile_stuff ? false : true);

    fprintf(config, "%s %d\n", "allow_removed_tracks", (int)allow_removed_tracks);

    fprintf(config, "%s %d\n", "enable_fuzzy_seek", (int)fuzzy_seek);
    fprintf(config, "%s %d\n", "use_shader_cache", (int)use_shader_cache);
    fprintf(config, "%s %d\n", "enable_mvp_optimization", (int)enable_mvp_optimization);
    fprintf(config, "%s %d\n", "enable_bones_optimization", (int)enable_bones_optimization);
    fclose(config);
  }
}

bool areButtonsLoaded = false;
void loadButtons(void) {
  sceClibMemset(controls_map, 0, CONTROLS_MAPPINGS_NUM * sizeof(int));
  sceClibMemset(secondary_controls_map, 0, CONTROLS_MAPPINGS_NUM * sizeof(int));
  char str[256], map[64], val[64];
  int ctrl_idx = 0;
  if (!areButtonsLoaded) {
    FILE *config = fopen(CONTROLS_FILE_PATH, "r");
    while (fgets(str, 256, config)) {
      if (str[0] == ';' || str[0] == '\n') continue;
      sscanf(str, "%s %s", map, val);
      for (int i = 0; i < CONTROLS_MAPPINGS_NUM; i++) {
        if (!strcmp(map, ControlsMapVar[i])) {
          for (int j = 0; j < CONTROLS_NUM; j++) {
            if (!strcmp(val, ControlsVar[j])) {
              ctrl_idx = j;
              break;
            }
          }
          if (controls_map[i] && controls_map[i] != ctrl_idx)
              secondary_controls_map[i] = ctrl_idx;
          else
              controls_map[i] = ctrl_idx;
          break;
        }
      }
    }
    areButtonsLoaded = true;
    fclose(config);
  }
  
  sceClibMemcpy(backup_controls_map, controls_map, CONTROLS_MAPPINGS_NUM * sizeof(int));
  sceClibMemcpy(backup_secondary_controls_map, secondary_controls_map, CONTROLS_MAPPINGS_NUM * sizeof(int));
}

void saveButtons(void) {
  FILE *config = fopen(CONTROLS_FILE_PATH, "w+");
  
  fprintf(config, "%s\n", "; Vita-enhanced controls");
  fprintf(config, "\n");
  
  for (int i = 0; i < CONTROLS_MAPPINGS_NUM; i++) {
    fprintf(config, "%s %s\n", ControlsMapVar[i], ControlsVar[controls_map[i]]);
    if (secondary_controls_map[i] && secondary_controls_map[i] != controls_map[i])
      fprintf(config, "%s %s\n", ControlsMapVar[i], ControlsVar[secondary_controls_map[i]]);	
  }
  
  fclose(config);
}

char *options_descs[] = {
  "Deadzone in pixels to use between inputs on both rearpad and touchscreen.\nThe default value is: 100.", // touch_x_margin
  "When enabled, L2/R2 will be mapped to the top of the front touchpad instead of the rear.\nThe default value is: Disabled.", // front_touch_triggers
  "Makes it possible to move the camera with the right stick when using a flying vehicle (Planes and helicopters).\nThe default value is: Enabled.", // fix_heli_plane_camera

  "Select the desired post processing effect filter to apply to the 3D rendering.\nThe default value is: PS2.", // skygfx_colorfilter
  "Enables shading effects that resamble the PS2 build.\nThe default value is: Enabled.", // skygfx_ps2_shading
  "Enables corona sun effect that resambles the PS2 build.\nThe default value is: Enabled.", // skygfx_ps2_sun

  "Anti-Aliasing is a technique used to reduce graphical artifacts surrounding 3D models. Greatly improves graphics quality at the cost of some GPU power.\nThe default value is: MSAA 2x.", // aa_mode
  "When enabled, high detail player textures are used.\nThe default value is: Disabled.", // enable_high_detail_player
  "When enabled, detail textures will be rendered.\nThe default value is: Disabled.", // disable_detail_textures
  "When enabled, mipmaps will have more precise bias adjustments at the cost of more expensive GPU code execution.\nThe default value is: Disabled.", // disable_tex_bias
  "When enabled, mipmaps will be used causing an higher memory usage and CPU usage but lower memory bandwidth over GPU.\nThe default value is: Enabled.", // disable_mipmaps
  "Makes hardware accelerated skinning properly work. Fixes broken animations especially noticeable in facial animations.\nThe default value is: Enabled.", // fix_skin_weights
  "When enabled, peds will have specular lighting reflections applied to their models.\nThe default value is: Disabled.", // disable_ped_spec
  "When enabled, Mobile build widgets and windows will be shown (eg. App rating window, cutscene skip widgets, etc...)\nThe default value is: Disabled.", // ignore_mobile_stuff

  "Allows the game to play removed tracks when using modded audio files.\nThe default value is: Disabled.", // allow_removed_tracks

  "When enabled, MP3 audio loading may be faster but less accurate.\nThe default value is: Disabled.", // enable_fuzzy_seek
  "Makes compiled shaders be cached on storage for subsequent usage. When enabled, the game will stutter on very first time a shader is compiled but will make the game have more fluid gameplay later.\nThe default value is: Enabled.", // use_shader_cache
  "Moves MVP calculation from GPU to CPU. May improve performances.\nThe default value is: Disabled.", // enable_mvp_optimization
  "Simplify GPU code related to bones calculations by preventing CPU to transpose the related matrix.\nThe default value is: Disabled.", // enable_bones_optimization
};

enum {
  OPT_DEADZONE,
  OPT_FRONT_TOUCH_TRIGGERS,
  OPT_FLYING_VEHICLES_FIX,

  OPT_COLOR_FILTER,
  OPT_PS2_SHADING,
  OPT_PS2_SUN,

  OPT_ANTIALIASING,
  OPT_HI_DETAIL_PLAYER,
  OPT_DETAIL_TEX,
  OPT_TEX_BIAS,
  OPT_MIPMAPS,
  OPT_SKINNING_FIX,
  OPT_PED_SPEC,
  OPT_MOBILE_STUFF,

  OPT_ALLOW_REMOVED_TRACKS,

  OPT_FUZZY_SEEK,
  OPT_SHADER_CACHE,
  OPT_MVP_OPT,
  OPT_BONES_OPT
};

char *desc = nullptr;

void SetDescription(int i) {
  if (ImGui::IsItemHovered())
    desc = options_descs[i];
}

int main(int argc, char *argv[]) {
  loadConfig();
  int exit_code = 0xDEAD;

  vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);
  ImGui::CreateContext();
  ImGui_ImplVitaGL_Init();
  ImGui_ImplVitaGL_TouchUsage(false);
  ImGui_ImplVitaGL_GamepadUsage(true);
  ImGui::StyleColorsDark();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

  ImGui::GetIO().MouseDrawCursor = false;

  while (exit_code == 0xDEAD) {
    desc = nullptr;
    ImGui_ImplVitaGL_NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
    ImGui::SetNextWindowSize(ImVec2(960, 544), ImGuiSetCond_Always);
    ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

    ImGui::TextColored(ImVec4(255, 255, 0, 255), "Inputs");
    ImGui::Text("Touchpanels Deadzone:"); ImGui::SameLine();
    ImGui::SliderInt("##touch_x_margin", &touch_x_margin, 0, 256);
    SetDescription(OPT_DEADZONE);
    ImGui::Text("Front Touchpad L2/R2:"); ImGui::SameLine();
    ImGui::Checkbox("##check14", &front_touch_triggers);
    SetDescription(OPT_FRONT_TOUCH_TRIGGERS);
    ImGui::Text("Flying Vehicles Camera Fix:"); ImGui::SameLine();
    ImGui::Checkbox("##check1", &fix_heli_plane_camera);
    SetDescription(OPT_FLYING_VEHICLES_FIX);
    ImGui::PopStyleVar();
    if (ImGui::Button("Configure Controls")) {
      show_controls_window = !show_controls_window;
      if (show_controls_window) loadButtons();
    }
    ImGui::Separator();

    ImGui::TextColored(ImVec4(255, 255, 0, 255), "Skygfx");
    ImGui::Text("PostFX Colour Filter:"); ImGui::SameLine();
    if (ImGui::BeginCombo("##combo", SkyGfxColorFilterName[skygfx_colorfilter])) {
      for (int n = 0; n < SKYGFX_COLOR_FILTER_NUM; n++) {
        bool is_selected = skygfx_colorfilter == n;
        if (ImGui::Selectable(SkyGfxColorFilterName[n], is_selected))
          skygfx_colorfilter = n;
        SetDescription(OPT_COLOR_FILTER);
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    SetDescription(OPT_COLOR_FILTER);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::Text("PS2 Shading:"); ImGui::SameLine();
    ImGui::Checkbox("##check2", &skygfx_ps2_shading);
    SetDescription(OPT_PS2_SHADING);
    ImGui::Text("PS2 Corona Sun:"); ImGui::SameLine();
    ImGui::Checkbox("##check3", &skygfx_ps2_sun);
    SetDescription(OPT_PS2_SUN);
    ImGui::Separator();
    ImGui::PopStyleVar();

    ImGui::TextColored(ImVec4(255, 255, 0, 255), "Graphics");
    ImGui::Text("Anti-Aliasing:"); ImGui::SameLine();
    if (ImGui::BeginCombo("##combo2", AntiAliasingName[aa_mode])) {
      for (int n = 0; n < ANTI_ALIASING_NUM; n++) {
        bool is_selected = aa_mode == n;
        if (ImGui::Selectable(AntiAliasingName[n], is_selected))
          aa_mode = n;
        SetDescription(OPT_ANTIALIASING);
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    SetDescription(OPT_ANTIALIASING);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::Text("High Detail Player Textures:"); ImGui::SameLine();
    ImGui::Checkbox("##check15", &high_detail_player);
    SetDescription(OPT_HI_DETAIL_PLAYER);
    ImGui::Text("Detail Textures:"); ImGui::SameLine();
    ImGui::Checkbox("##check4", &detail_textures);
    SetDescription(OPT_DETAIL_TEX);
    ImGui::Text("Texture Bias:"); ImGui::SameLine();
    ImGui::Checkbox("##check5", &tex_bias);
    SetDescription(OPT_TEX_BIAS);
    ImGui::Text("Mipmaps:"); ImGui::SameLine();
    ImGui::Checkbox("##check6", &mipmaps);
    SetDescription(OPT_MIPMAPS);
    ImGui::Text("Skinning Fix:"); ImGui::SameLine();
    ImGui::Checkbox("##check7", &fix_skin_weights);
    SetDescription(OPT_SKINNING_FIX);
    ImGui::Text("Peds Reflections:"); ImGui::SameLine();
    ImGui::Checkbox("##check8", &ped_spec);
    SetDescription(OPT_PED_SPEC);
    ImGui::Text("Mobile Widgets:"); ImGui::SameLine();
    ImGui::Checkbox("##check11", &mobile_stuff);
    SetDescription(OPT_MOBILE_STUFF);
    ImGui::Separator();

    ImGui::TextColored(ImVec4(255, 255, 0, 255), "Audio");
    ImGui::Text("Allow removed tracks:"); ImGui::SameLine();
    ImGui::Checkbox("##check17", &allow_removed_tracks);
    SetDescription(OPT_ALLOW_REMOVED_TRACKS);
    ImGui::Separator();

    ImGui::TextColored(ImVec4(255, 255, 0, 255), "Optimizations");
    ImGui::Text("MP3 Fuzzy Seek:"); ImGui::SameLine();
    ImGui::Checkbox("##check16", &fuzzy_seek);
    SetDescription(OPT_FUZZY_SEEK);
    ImGui::Text("Shader Cache:"); ImGui::SameLine();
    ImGui::Checkbox("##check9", &use_shader_cache);
    SetDescription(OPT_SHADER_CACHE);
    ImGui::Text("MVP Optimization:"); ImGui::SameLine();
    ImGui::Checkbox("##check12", &enable_mvp_optimization);
    SetDescription(OPT_MVP_OPT);
    ImGui::Text("Bones Optimization:"); ImGui::SameLine();
    ImGui::Checkbox("##check13", &enable_bones_optimization);
    SetDescription(OPT_BONES_OPT);
    ImGui::PopStyleVar();
    ImGui::Separator();

    if (ImGui::Button("Save and Exit"))
      exit_code = 0;
    ImGui::SameLine();
    if (ImGui::Button("Save and Launch the game"))
      exit_code = 1;
    ImGui::SameLine();
    if (ImGui::Button("Discard and Exit"))
      exit_code = 2;
    ImGui::SameLine();
    if (ImGui::Button("Discard and Launch the game"))
      exit_code = 3;
    ImGui::Separator();

    if (desc) {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::TextWrapped(desc);
    }
    ImGui::End();
  
    if (show_controls_window) {
      ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
      ImGui::SetNextWindowSize(ImVec2(960, 544), ImGuiSetCond_Always);
      ImGui::Begin("Controls Configuration", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
      ImGui::SetCursorPosX(320);
      ImGui::PushItemWidth(150);
      if (ImGui::Button("Save Changes")) {
        show_controls_window = !show_controls_window;
        saveButtons();
      }
      ImGui::SameLine();
      if (ImGui::Button("Discard Changes")) {
        show_controls_window = !show_controls_window;
        sceClibMemcpy(controls_map, backup_controls_map, CONTROLS_MAPPINGS_NUM * sizeof(int));
        sceClibMemcpy(secondary_controls_map, backup_secondary_controls_map, CONTROLS_MAPPINGS_NUM * sizeof(int));
      }
      ImGui::PopItemWidth();
      ImGui::Columns(2, nullptr, false);
      ImGui::SetColumnWidth(0, 300);
      for (int i = 0; i < CONTROLS_MAPPINGS_NUM; i++) {
        std::string text = ControlsMapName[i];
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text.c_str()).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::Text(ControlsMapName[i]);
        ImGui::NextColumn();
        char cname[16];
        sprintf(cname, "##comboc%d", i);
        ImGui::PushItemWidth(ImGui::CalcTextSize("Analog Right (X Axis)").x + ImGui::GetStyle().FramePadding.x * 2.0f + 30);
        if (ImGui::BeginCombo(cname, ControlsName[controls_map[i]])) {
          for (int n = 0; n < CONTROLS_NUM; n++) {
            bool is_selected = controls_map[i] == n;
            if (ImGui::Selectable(ControlsName[n], is_selected))
              controls_map[i] = n;
            if (is_selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        ImGui::SameLine();
        sprintf(cname, "##combosc%d", i);
        if (ImGui::BeginCombo(cname, ControlsName[secondary_controls_map[i]])) {
          for (int n = 0; n < CONTROLS_NUM; n++) {
            bool is_selected = secondary_controls_map[i] == n;
            if (ImGui::Selectable(ControlsName[n], is_selected))
              secondary_controls_map[i] = n;
            if (is_selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();
        
      }
      ImGui::Columns(1);
      ImGui::End();
    }
  
    glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
    ImGui::Render();
    ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
    vglSwapBuffers(GL_FALSE);
  }

  if (exit_code < 2) // Save
    saveConfig();

  if (exit_code % 2 == 1) // Launch
    sceAppMgrLoadExec("app0:/eboot.bin", NULL, NULL);

  return 0;
}

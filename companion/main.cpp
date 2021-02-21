#include <vitasdk.h>
#include <vitaGL.h>
#include <imgui_vita.h>
#include <stdio.h>

#define CONFIG_FILE_PATH "ux0:data/gtasa/config.txt"
#define NUM_OPTIONS 17

int _newlib_heap_size_user = 192 * 1024 * 1024;

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

int touch_x_margin = 100;
bool front_touch_triggers = false;
bool fix_heli_plane_camera = true;
bool fix_skin_weights = true;
bool fix_map_bottleneck = true;
bool use_shader_cache = true;
bool skygfx_ps2_shading = true;
bool mobile_stuff = false;
int skygfx_colorfilter = SKYGFX_COLOR_FILTER_PS2;
int aa_mode = SCE_GXM_MULTISAMPLE_2X;
bool skygfx_ps2_sun = true;
bool detail_textures = false;
bool ped_spec = false;
bool tex_bias = false;
bool mipmaps = true;
bool enable_mvp_optimization = false;
bool enable_bones_optimization = false;

void loadConfig(void) {
  char buffer[30];
  int value;

  FILE *config = fopen(CONFIG_FILE_PATH, "r");

  if (config) {
    while (EOF != fscanf(config, "%[^ ] %d\n", buffer, &value)) {
      if (strcmp("touch_x_margin", buffer) == 0) touch_x_margin = value;
      else if (strcmp("front_touch_triggers", buffer) == 0) front_touch_triggers = (bool)value;
      else if (strcmp("fix_heli_plane_camera", buffer) == 0) fix_heli_plane_camera = (bool)value;
      else if (strcmp("ignore_mobile_stuff", buffer) == 0) mobile_stuff = value ? false : true;

      else if (strcmp("fix_skin_weights", buffer) == 0) fix_skin_weights = (bool)value;
      else if (strcmp("fix_map_bottleneck", buffer) == 0) fix_map_bottleneck = (bool)value;
      else if (strcmp("use_shader_cache", buffer) == 0) use_shader_cache = (bool)value;
      else if (strcmp("aa_mode", buffer) == 0) aa_mode = value;
      else if (strcmp("enable_mvp_optimization", buffer) == 0) enable_mvp_optimization = (bool)value;
      else if (strcmp("enable_bones_optimization", buffer) == 0) enable_bones_optimization = (bool)value;

      else if (strcmp("skygfx_ps2_shading", buffer) == 0) skygfx_ps2_shading = (bool)value;
      else if (strcmp("skygfx_colorfilter", buffer) == 0) skygfx_colorfilter = value;
      else if (strcmp("skygfx_ps2_sun", buffer) == 0) skygfx_ps2_sun = (bool)value;

      else if (strcmp("disable_detail_textures", buffer) == 0) detail_textures = value ? false : true;
      else if (strcmp("disable_ped_spec", buffer) == 0) ped_spec = value ? false : true;
      else if (strcmp("disable_tex_bias", buffer) == 0) tex_bias = value ? false : true;
      else if (strcmp("disable_mipmaps", buffer) == 0) mipmaps = value ? false : true;
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
    fprintf(config, "%s %d\n", "ignore_mobile_stuff", mobile_stuff ? false : true);

    fprintf(config, "%s %d\n", "fix_skin_weights", (int)fix_skin_weights);
    fprintf(config, "%s %d\n", "fix_map_bottleneck", (int)fix_map_bottleneck);
    fprintf(config, "%s %d\n", "use_shader_cache", (int)use_shader_cache);
    fprintf(config, "%s %d\n", "aa_mode", aa_mode);
    fprintf(config, "%s %d\n", "enable_mvp_optimization", (int)enable_mvp_optimization);
    fprintf(config, "%s %d\n", "enable_bones_optimization", (int)enable_bones_optimization);

    fprintf(config, "%s %d\n", "skygfx_ps2_shading", (int)skygfx_ps2_shading);
    fprintf(config, "%s %d\n", "skygfx_colorfilter", skygfx_colorfilter);
    fprintf(config, "%s %d\n", "skygfx_ps2_sun", (int)skygfx_ps2_sun);

    fprintf(config, "%s %d\n", "disable_detail_textures", detail_textures ? false : true);
    fprintf(config, "%s %d\n", "disable_ped_spec", ped_spec ? false : true);

    fprintf(config, "%s %d\n", "disable_tex_bias", tex_bias ? false : true);
    fprintf(config, "%s %d\n", "disable_mipmaps", mipmaps ? false : true);
    fclose(config);
  }
}

char *options_descs[NUM_OPTIONS] = {
  "Deadzone in pixels to use between inputs on both rearpad and touchscreen.\nThe default value is: 100.", // touch_x_margin
  "When enabled, L2/R2 will be mapped to the top of the front touchpad instead of the rear.\nThe default value is: Disabled.", // front_touch_triggers
  "Makes it possible to move the camera with the right stick when using a flying vehicle (Planes and helicopters).\nThe default value is: Enabled.", // fix_heli_plane_camera
  "Select the desired post processing effect filter to apply to the 3D rendering.\nThe default value is: PS2.", // skygfx_colorfilter
  "Enables shading effects that resamble the PS2 build.\nThe default value is: Enabled.", // skygfx_ps2_shading
  "Enables corona sun effect that resambles the PS2 build.\nThe default value is: Enabled.", // skygfx_ps2_sun
  "When enabled, detail textures will be rendered.\nThe default value is: Disabled.", // disable_detail_textures
  "When enabled, mipmaps will have more precise bias adjustments at the cost of more expensive GPU code execution.\nThe default value is: Disabled.", // disable_tex_bias
  "When enabled, mipmaps will be used causing an higher memory usage and CPU usage but lower memory bandwidth over GPU.\nThe default value is: Enabled.", // disable_mipmaps
  "Makes hardware accelerated skinning properly work. Fixes broken animations especially noticeable in facial animations.\nThe default value is: Enabled.", // fix_skin_weights
  "When enabled, peds will have specular lighting reflections applied to their models.\nThe default value is: Disabled.", // disable_ped_spec
  "Makes compiled shaders be cached on storage for subsequent usage. When enabled, the game will stutter on very first time a shader is compiled but will make the game have more fluid gameplay later.\nThe default value is: Enabled.", // use_shader_cache
  "Removes regions highlighting in the pause menu map. This will fix the performance issues during advanced stages of the game in pause menu.\nThe default value is: Enabled.", // fix_map_bottleneck
  "Anti-Aliasing is a technique used to reduce graphical artifacts surrounding 3D models. Greatly improves graphics quality at the cost of some GPU power.\nThe default value is: MSAA 2x.", // aa_mode
  "When enabled, Mobile build widgets and windows will be shown (eg. App rating window, cutscene skip widgets, etc...)\nThe default value is: Disabled.", // ignore_mobile_stuff
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
  OPT_DETAIL_TEX,
  OPT_TEX_BIAS,
  OPT_MIPMAPS,
  OPT_SKINNING_FIX,
  OPT_PED_SPEC,
  OPT_SHADER_CACHE,
  OPT_MAP_FIX,
  OPT_ANTIALIASING,
  OPT_MOBILE_STUFF,
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
    ImGui::Begin("Grand Theft Auto: San Andreas - Config Manager", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

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
    ImGui::Separator();
    ImGui::PopStyleVar();

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

    ImGui::TextColored(ImVec4(255, 255, 0, 255), "Optimizations");
    ImGui::Text("Shader Cache:"); ImGui::SameLine();
    ImGui::Checkbox("##check9", &use_shader_cache);
    SetDescription(OPT_SHADER_CACHE);
    ImGui::Text("Map Bottleneck Fix:"); ImGui::SameLine();
    ImGui::Checkbox("##check10", &fix_map_bottleneck);
    SetDescription(OPT_MAP_FIX);
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

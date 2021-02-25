/* main.c -- Grand Theft Auto: San Andreas .so loader
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/touch.h>
#include <taihen.h>
#include <kubridge.h>
#include <vitashark.h>
#include <vitaGL.h>

#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <math.h>
#include <math_neon.h>

#include <errno.h>
#include <ctype.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "main.h"
#include "config.h"
#include "dialog.h"
#include "fios.h"
#include "so_util.h"
#include "jni_patch.h"
#include "openal_patch.h"
#include "opengl_patch.h"
#include "gfx_patch.h"
#include "sha1.h"

#include "libc_bridge.h"

int sceLibcHeapSize = MEMORY_SCELIBC_MB * 1024 * 1024;
int _newlib_heap_size_user = MEMORY_NEWLIB_MB * 1024 * 1024;

SceTouchPanelInfo panelInfoFront, panelInfoBack;

int debugPrintf(char *text, ...) {
#ifdef DEBUG
  va_list list;
  char string[512];

  va_start(list, text);
  vsprintf(string, text, list);
  va_end(list);

  SceUID fd = sceIoOpen("ux0:data/gtasa_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
  if (fd >= 0) {
    sceIoWrite(fd, string, strlen(string));
    sceIoClose(fd);
  }
#endif
  return 0;
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
#ifdef DEBUG
  va_list list;
  char string[512];

  va_start(list, fmt);
  vsprintf(string, fmt, list);
  va_end(list);

  debugPrintf("%s: %s\n", tag, string);
#endif
  return 0;
}

int ret0(void) {
  return 0;
}

int ret1(void) {
  return 1;
}

int mkdir(const char *pathname, mode_t mode) {
  if (sceIoMkdir(pathname, mode) < 0)
    return -1;
  return 0;
}

int OS_SystemChip(void) {
  return 19; // default
}

int OS_ScreenGetHeight(void) {
  return SCREEN_H;
}

int OS_ScreenGetWidth(void) {
  return SCREEN_W;
}

// only used for NVEventAppMain
int pthread_create_fake(int r0, int r1, int r2, void *arg) {
  int (* func)() = *(void **)(arg + 4);
  return func();
}

int pthread_mutex_init_fake(SceKernelLwMutexWork **work) {
  *work = (SceKernelLwMutexWork *)memalign(8, sizeof(SceKernelLwMutexWork));
  if (sceKernelCreateLwMutex(*work, "mutex", SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, NULL) < 0)
    return -1;
  return 0;
}

int pthread_mutex_destroy_fake(SceKernelLwMutexWork **work) {
  if (sceKernelDeleteLwMutex(*work) < 0)
    return -1;
  free(*work);
  return 0;
}

int pthread_mutex_lock_fake(SceKernelLwMutexWork **work) {
  if (!*work)
    pthread_mutex_init_fake(work);
  if (sceKernelLockLwMutex(*work, 1, NULL) < 0)
    return -1;
  return 0;
}

int pthread_mutex_unlock_fake(SceKernelLwMutexWork **work) {
  if (sceKernelUnlockLwMutex(*work, 1) < 0)
    return -1;
  return 0;
}

int sem_init_fake(int *uid) {
  *uid = sceKernelCreateSema("sema", 0, 0, 0x7fffffff, NULL);
  if (*uid < 0)
    return -1;
  return 0;
}

int sem_post_fake(int *uid) {
  if (sceKernelSignalSema(*uid, 1) < 0)
    return -1;
  return 0;
}

int sem_wait_fake(int *uid) {
  if (sceKernelWaitSema(*uid, 1, NULL) < 0)
    return -1;
  return 0;
}

int sem_destroy_fake(int *uid) {
  if (sceKernelDeleteSema(*uid) < 0)
    return -1;
  return 0;
}

int thread_stub(SceSize args, uintptr_t *argp) {
  int (* func)(void *arg) = (void *)argp[0];
  void *arg = (void *)argp[1];
  char *out = (char *)argp[2];
  out[0x41] = 1; // running
  func(arg);
  return sceKernelExitDeleteThread(0);
}

// CdStream with cpu 3 and priority 3
// RenderQueue with cpu 2 and priority 3
// MainThread with cpu 1 and priority 2
// StreamThread with cpu 3 and priority 1
// BankLoader with cpu 4 and priority 0
void *OS_ThreadLaunch(int (* func)(), void *arg, int cpu, char *name, int unused, int priority) {
  int vita_priority;
  int vita_affinity;

  switch (priority) {
    case 0:
      vita_priority = 68;
      break;
    case 1:
      vita_priority = 67;
      break;
    case 2:
      vita_priority = 66;
      break;
    case 3:
      vita_priority = 65;
      break;
    default:
      vita_priority = 0x10000100;
      break;
  }

  switch (cpu) {
    case 1:
      vita_affinity = 0x10000;
      break;
    case 2:
      vita_affinity = 0x20000;
      break;
    case 3:
      vita_affinity = 0x40000;
      break;
    case 4:
      vita_affinity = 0x40000;
      break;
    default:
      vita_affinity = 0;
      break;
  }

  SceUID thid = sceKernelCreateThread(name, (SceKernelThreadEntry)thread_stub, vita_priority, 128 * 1024, 0, vita_affinity, NULL);
  if (thid >= 0) {
    char *out = malloc(0x48);
    *(int *)(out + 0x24) = thid;

    uintptr_t args[3];
    args[0] = (uintptr_t)func;
    args[1] = (uintptr_t)arg;
    args[2] = (uintptr_t)out;
    sceKernelStartThread(thid, sizeof(args), args);

    return out;
  }

  return NULL;
}

void OS_ThreadWait(void *thread) {
  if (thread)
    sceKernelWaitThreadEnd(*(int *)(thread + 0x24), NULL, NULL);
}

void *OS_ThreadSetValue(void *RenderQueue) {
  *(uint8_t *)(RenderQueue + 601) = 0;
  return NULL;
}

#define HARRIER_NOZZLE_ROTATE_LIMIT 5000
#define HARRIER_NOZZLE_ROTATERATE 25.0f

static void *(* CPad__GetPad)(int pad);
static int (* CPad__GetCarGunUpDown)(void *pad, int r1, int r2, float r3, int r4);

static float *CTimer__ms_fTimeStep;

void CPlane_ProcessControlInputs_Harrier(void *this, int pad) {
  uint16_t modelIndex = *(uint16_t *)(this + 0x26);
  if (modelIndex == 520) {
    float rightStickY = (float)CPad__GetCarGunUpDown(CPad__GetPad(pad), 0, 0, 2500.0f, 0);
    if (fabsf(rightStickY) > 10.0f) {
      *(int16_t *)(this + 0x882) = *(int16_t *)(this + 0x880);
      *(int16_t *)(this + 0x880) += (int16_t)(rightStickY / 128.0f * HARRIER_NOZZLE_ROTATERATE * *CTimer__ms_fTimeStep);
      if (*(int16_t *)(this + 0x880) < 0)
        *(int16_t *)(this + 0x880) = 0;
      else if (*(int16_t *)(this + 0x880) > HARRIER_NOZZLE_ROTATE_LIMIT)
        *(int16_t *)(this + 0x880) = HARRIER_NOZZLE_ROTATE_LIMIT;
    }
  }
}

__attribute__((naked)) void CPlane_ProcessControlInputs_Harrier_stub(void) {
  asm volatile(
    "push {r0-r11}\n"
    "mov r0, r4\n"
    "mov r1, r8\n"
    "bl CPlane_ProcessControlInputs_Harrier\n"
  );

  register uintptr_t retAddr asm ("r12") = (uintptr_t)text_base + 0x005765F0 + 0x1;

  asm volatile(
    "pop {r0-r11}\n"
    "bx %0\n"
  :: "r" (retAddr));
}

int CCam__Process_FollowCar_SA_camSetArrPos(void *this) {
  uint16_t modelIndex = *(uint16_t *)(this + 0x26);
  if (modelIndex == 520 && *(int16_t *)(this + 0x880) >= 3000) {
    return 2; // heli
  } else {
    return modelIndex == 539 ? 0 : 3; // car or plane
  }
}

__attribute__((naked)) void CCam__Process_FollowCar_SA_camSetArrPos_stub(void) {
  asm volatile(
    "push {r0-r8, r10-r11}\n"
    "mov r0, r11\n"
    "bl CCam__Process_FollowCar_SA_camSetArrPos\n"
    "mov r9, r0\n"
  );

  register uintptr_t retAddr asm ("r12") = (uintptr_t)text_base + 0x003C033A + 0x1;

  asm volatile(
    "pop {r0-r8, r10-r11}\n"
    "bx %0\n"
  :: "r" (retAddr));
}

uint64_t CCam__Process_FollowCar_SA_yMovement(void *this, uint32_t xMovement, uint32_t yMovement) {
  uint16_t modelIndex = *(uint16_t *)(this + 0x26);
  switch (modelIndex) {
    // case 564:
      // xMovement = 0;
    // // Fall-through
    // case 406:
    // case 443:
    // case 486:
    case 520:
    // case 524:
    // case 525:
    // case 530:
    // case 531:
    // case 592:
      yMovement = 0;
      break;
    default:
      break;
  }

  return (uint64_t)xMovement | (uint64_t)yMovement << 32;
}

__attribute__((naked)) void CCam__Process_FollowCar_SA_yMovement_stub(void) {
  asm volatile(
    "push {r0-r11}\n"
    "mov r0, r11\n"
    "vmov r1, s21\n"
    "vmov r2, s28\n"
    "bl CCam__Process_FollowCar_SA_yMovement\n"
    "vmov s28, r1\n"
    "vmov s21, r0\n"
  );

  register uintptr_t retAddr asm ("r12") = (uintptr_t)text_base + 0x003C1308 + 0x1;

  asm volatile(
    "pop {r0-r11}\n"
    "bx %0\n"
  :: "r" (retAddr));
}

typedef enum {
  MATRIX_PROJ_ID,
  MATRIX_VIEW_ID,
  MATRIX_OBJ_ID,
  MATRIX_TEX_ID
} RQShaderMatrixConstantID;

static void *(* GetCurrentProjectionMatrix)();
static void *(* GetCurrentViewMatrix)();
static void *(* GetCurrentObjectMatrix)();

void SetMatrixConstant(void *this, RQShaderMatrixConstantID id, float *matrix) {
  void *UniformMatrix = this + 0x4C * id;
  float *UniformMatrixData = UniformMatrix + 0x2AC;
  if (sceClibMemcmp(UniformMatrixData, matrix, 16 * 4) != 0) {
    sceClibMemcpy(UniformMatrixData, matrix, 16 * 4);
    *(uint8_t *)(UniformMatrix + 0x2A8) = 1;
    *(uint8_t *)(UniformMatrix + 0x2EC) = 1;
  }
}

void ES2Shader__SetMatrixConstant(void *this, RQShaderMatrixConstantID id, float *matrix) {
  if (id == MATRIX_TEX_ID) {
    SetMatrixConstant(this, id, matrix);
  } else {
    float *ProjMatrix = GetCurrentProjectionMatrix();
    float *ViewMatrix = GetCurrentViewMatrix();
    float *ObjMatrix = GetCurrentObjectMatrix();

    int forced = ((id == MATRIX_PROJ_ID) && !((uint8_t *)ProjMatrix)[64]) ||
                 ((id == MATRIX_VIEW_ID) && !((uint8_t *)ViewMatrix)[64]) ||
                 ((id == MATRIX_OBJ_ID) && !((uint8_t *)ObjMatrix)[64]);
    if (forced || ((uint8_t *)ProjMatrix)[64] || ((uint8_t *)ViewMatrix)[64] || ((uint8_t *)ObjMatrix)[64]) {
      float mv[16], mvp[16];
      matmul4_neon(ViewMatrix, ObjMatrix, mv);
      matmul4_neon(ProjMatrix, mv, mvp);

      SetMatrixConstant(this, MATRIX_PROJ_ID, mvp);
    }

    if (forced || ((uint8_t *)ViewMatrix)[64])
       SetMatrixConstant(this, MATRIX_VIEW_ID, ViewMatrix);
    if (forced || ((uint8_t *)ObjMatrix)[64])
       SetMatrixConstant(this, MATRIX_OBJ_ID, ObjMatrix);

    ((uint8_t *)ProjMatrix)[64] = 0;
    ((uint8_t *)ViewMatrix)[64] = 0;
    ((uint8_t *)ObjMatrix)[64] = 0;
  }
}

// size of skin_map is 128 * 4 * 4 * 3 = 0x1800
static float *skin_map;
static int *skin_dirty;
static int *skin_num;

int emu_InternalSkinGetVectorCount(void) {
  return 4 * *skin_num;
}

void SkinSetMatrices(void *skin, float *matrix) {
  int num = *(int *)(skin + 4);
  sceClibMemcpy(skin_map, matrix, 64 * num);
  *skin_dirty = 1;
  *skin_num = num;
}

// War Drum added new cheats without updating the hash key table /facepalm
static uint32_t CCheat__m_aCheatHashKeys[] = {
  0xDE4B237D, 0xB22A28D1, 0x5A783FAE,
  // WEAPON4, TIMETRAVEL, SCRIPTBYPASS, SHOWMAPPINGS
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  // INVINCIBILITY, SHOWTAPTOTARGET, SHOWTARGETING
  0x00000000, 0x00000000, 0x00000000,
  0xEECCEA2B,
  0x42AF1E28, 0x555FC201, 0x2A845345, 0xE1EF01EA,
  0x771B83FC, 0x5BF12848, 0x44453A17, 0x00000000,
  0xB69E8532, 0x8B828076, 0xDD6ED9E9, 0xA290FD8C,
  0x00000000, 0x43DB914E, 0xDBC0DD65, 0x00000000,
  0xD08A30FE, 0x37BF1B4E, 0xB5D40866, 0xE63B0D99,
  0x675B8945, 0x4987D5EE, 0x2E8F84E8, 0x00000000,
  0x00000000, 0x0D5C6A4E, 0x00000000, 0x00000000,
  0x66516EBC, 0x4B137E45, 0x00000000, 0x00000000,
  0x3A577325, 0xD4966D59, 0x5FD1B49D, 0xA7613F99,
  0x1792D871, 0xCBC579DF, 0x4FEDCCFF, 0x44B34866,
  0x2EF877DB, 0x2781E797, 0x2BC1A045, 0xB2AFE368,
  0x00000000, 0x00000000, 0x1A5526BC, 0xA48A770B,
  0x00000000, 0x00000000, 0x00000000, 0x7F80B950,
  0x6C0FA650, 0xF46F2FA4, 0x70164385,
  // PREDATOR
  0x00000000,
  0x00000000,
  0x885D0B50, 0x151BDCB3, 0xADFA640A, 0xE57F96CE,
  0x040CF761, 0xE1B33EB9, 0xFEDA77F7, 0x00000000,
  0x00000000, 0xF53EF5A5, 0xF2AA0C1D, 0xF36345A8,
  0x00000000, 0xB7013B1B, 0x00000000, 0x31F0C3CC,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0xF01286E9, 0xA841CC0A, 0x31EA09CF,
  0xE958788A, 0x02C83A7C, 0xE49C3ED4, 0x171BA8CC,
  0x86988DAE, 0x2BDD2FA1, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000,
};

static void (* CCheat__AddToCheatString)(char c);

char *input_keyboard = NULL;
int input_cheat = 0;

void CCheat__DoCheats(void) {
  if (input_keyboard) {
    for (int i = 0; input_keyboard[i]; i++)
      CCheat__AddToCheatString(toupper(input_keyboard[i]));
    input_keyboard = NULL;
  }
}

int ProcessEvents(void) {
  if (!input_cheat) {
    SceCtrlData pad;
    sceCtrlPeekBufferPositiveExt2(0, &pad, 1);
    if ((pad.buttons & SCE_CTRL_L1) && (pad.buttons & SCE_CTRL_SELECT)) {
      init_ime_dialog("Insert cheat code", "");
      input_cheat = 1;
    }
  } else {
    input_keyboard = get_ime_dialog_result();
    if (input_keyboard)
      input_cheat = 0;
  }

  return 0; // 1 is exit!
}

static void *CHIDJoystickPS3__vtable;
static void *(* CHIDJoystick__CHIDJoystick)(void *this, const char *name);
static void (* CHIDJoystick__AddMapping)(void *this, int button_id, HIDMapping mapping);

void *CHIDJoystickPS3__CHIDJoystickPS3(void *this, const char *name) {
  CHIDJoystick__CHIDJoystick(this, name);
  *(uintptr_t *)this = (uintptr_t)CHIDJoystickPS3__vtable + 8;

  for (int i = 0; i < mapping_count; i++) {
    CHIDJoystick__AddMapping(this, button_mapping[i].button_id, button_mapping[i].hid_mapping);
  }

  return this;
}

int MainMenuScreen__OnExit(void) {
  return sceKernelExitProcess(0);
}

extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;

void patch_game(void) {
  *(int *)so_find_addr("UseCloudSaves") = 0;
  *(int *)so_find_addr("UseTouchSense") = 0;

  if (config.disable_detail_textures)
    *(int *)so_find_addr("gNoDetailTextures") = 1;

  if (config.fix_heli_plane_camera) {
    // Dummy all FindPlayerVehicle calls so the right analog stick can be used as camera again
    uint32_t movs_r0_0 = 0xBF002000;
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003C0866), &movs_r0_0, sizeof(movs_r0_0));
    // kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003C1652), &movs_r0_0, sizeof(movs_r0_0));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003C1518), &movs_r0_0, sizeof(movs_r0_0));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003C198A), &movs_r0_0, sizeof(movs_r0_0));

    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003FC462), &movs_r0_0, sizeof(movs_r0_0));
    // kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003FC482), &movs_r0_0, sizeof(movs_r0_0));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003FC754), &movs_r0_0, sizeof(movs_r0_0));
    // kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003FC774), &movs_r0_0, sizeof(movs_r0_0));

    // Fix Harrier thruster control
    hook_thumb((uintptr_t)(text_base + 0x003C057C), (uintptr_t)CCam__Process_FollowCar_SA_camSetArrPos_stub);
    hook_thumb((uintptr_t)(text_base + 0x003C12F4), (uintptr_t)CCam__Process_FollowCar_SA_yMovement_stub);

    CPad__GetPad = (void *)so_find_addr("_ZN4CPad6GetPadEi");
    CPad__GetCarGunUpDown = (void *)so_find_addr("_ZN4CPad15GetCarGunUpDownEbP11CAutomobilefb");
    CTimer__ms_fTimeStep = (float *)so_find_addr("_ZN6CTimer12ms_fTimeStepE");
    hook_thumb((uintptr_t)(text_base + 0x00576432), (uintptr_t)CPlane_ProcessControlInputs_Harrier_stub);
  }

  // Force using GL_UNSIGNED_SHORT
  if (config.fix_skin_weights) {
    uint16_t movs_r1_1 = 0x2101;
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x001C8064), &movs_r1_1, sizeof(movs_r1_1));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x001C8082), &movs_r1_1, sizeof(movs_r1_1));
  }

  if (config.enable_high_detail_player)
    hook_thumb(so_find_addr("_Z17UseHiDetailPlayerv"), (uintptr_t)ret1);

  if (config.enable_bones_optimization) {
    skin_map = (float *)so_find_addr("skin_map");
    skin_dirty = (int *)so_find_addr("skin_dirty");
    skin_num = (int *)so_find_addr("skin_num");
    hook_thumb(so_find_addr("_Z30emu_InternalSkinGetVectorCountv"), (uintptr_t)emu_InternalSkinGetVectorCount);
    hook_thumb((uintptr_t)text_base + 0x001C8670, (uintptr_t)SkinSetMatrices);
  }

  if (config.enable_mvp_optimization) {
    GetCurrentProjectionMatrix = (void *)so_find_addr("_Z26GetCurrentProjectionMatrixv");
    GetCurrentViewMatrix = (void *)so_find_addr("_Z20GetCurrentViewMatrixv");
    GetCurrentObjectMatrix = (void *)so_find_addr("_Z22GetCurrentObjectMatrixv");
    hook_thumb(so_find_addr("_ZN9ES2Shader17SetMatrixConstantE24RQShaderMatrixConstantIDPKf"), (uintptr_t)ES2Shader__SetMatrixConstant);
  }

  // Remove map highlight (explored regions) since it's rendered very inefficiently
  if (config.fix_map_bottleneck)
    hook_thumb((uintptr_t)(text_base + 0x002AADE0), (uintptr_t)(text_base + 0x002AAF9A + 0x1));

  // Ignore widgets and popups introduced in mobile
  if (config.ignore_mobile_stuff) {
    uint16_t nop16 = 0xbf00;
    uint32_t nop32 = 0xbf00bf00;

    // Ignore cutscene skip button
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x0043A7A0), &nop32, sizeof(nop32));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x004627E6), &nop32, sizeof(nop32));

    // Ignore steering control popup
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x003F91B6), &nop16, sizeof(nop16));

    // Ignore app rating popup
    hook_thumb(so_find_addr("_Z12Menu_ShowNagv"), (uintptr_t)ret0);

    // Ignore items in the controls menu
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x0029E4AE), &nop32, sizeof(nop32));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x0029E4E6), &nop32, sizeof(nop32));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x0029E50A), &nop32, sizeof(nop32));
    kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x0029E530), &nop32, sizeof(nop32));
  }

  hook_thumb(so_find_addr("__cxa_guard_acquire"), (uintptr_t)&__cxa_guard_acquire);
  hook_thumb(so_find_addr("__cxa_guard_release"), (uintptr_t)&__cxa_guard_release);

  hook_thumb(so_find_addr("_Z24NVThreadGetCurrentJNIEnvv"), (uintptr_t)NVThreadGetCurrentJNIEnv);

  // do not use pthread
  hook_thumb(so_find_addr("_Z15OS_ThreadLaunchPFjPvES_jPKci16OSThreadPriority"), (uintptr_t)OS_ThreadLaunch);
  hook_thumb(so_find_addr("_Z13OS_ThreadWaitPv"), (uintptr_t)OS_ThreadWait);

  // do not use mutex for RenderQueue
  hook_thumb(so_find_addr("_Z17OS_ThreadSetValuePv"), (uintptr_t)OS_ThreadSetValue);

  hook_thumb(so_find_addr("_Z17OS_ScreenGetWidthv"), (uintptr_t)OS_ScreenGetWidth);
  hook_thumb(so_find_addr("_Z18OS_ScreenGetHeightv"), (uintptr_t)OS_ScreenGetHeight);

  // TODO: set deviceChip, definedDevice
  hook_thumb(so_find_addr("_Z20AND_SystemInitializev"), (uintptr_t)ret0);

  // TODO: implement touch here
  hook_thumb(so_find_addr("_Z13ProcessEventsb"), (uintptr_t)ProcessEvents);

  // no adjustable
  hook_thumb(so_find_addr("_ZN14CAdjustableHUD10SaveToDiskEv"), (uintptr_t)ret0);
  hook_thumb(so_find_addr("_ZN15CTouchInterface27RepositionAdjustableWidgetsEv"), (uintptr_t)ret0);

  // cheats support
  CCheat__AddToCheatString = (void *)so_find_addr("_ZN6CCheat16AddToCheatStringEc");
  kuKernelCpuUnrestrictedMemcpy((void *)so_find_addr("_ZN6CCheat16m_aCheatHashKeysE"), CCheat__m_aCheatHashKeys, sizeof(CCheat__m_aCheatHashKeys));
  hook_thumb(so_find_addr("_ZN6CCheat8DoCheatsEv"), (uintptr_t)CCheat__DoCheats);

  // hook buttons mapping
  CHIDJoystickPS3__vtable = (void *)so_find_addr("_ZTV15CHIDJoystickPS3");
  CHIDJoystick__CHIDJoystick = (void *)so_find_addr("_ZN12CHIDJoystickC2EPKc");
  CHIDJoystick__AddMapping = (void *)so_find_addr("_ZN12CHIDJoystick10AddMappingEi10HIDMapping");
  hook_thumb(so_find_addr("_ZN15CHIDJoystickPS3C2EPKc"), (uintptr_t)CHIDJoystickPS3__CHIDJoystickPS3);

  // support graceful exit
  hook_thumb(so_find_addr("_ZN14MainMenuScreen6OnExitEv"), (uintptr_t)MainMenuScreen__OnExit);
}

void glTexImage2DHook(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * data) {
  if (level == 0)
    glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
}

void glCompressedTexImage2DHook(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data) {
  // mips for PVRTC textures break when they're under 1 block in size
  if (level == 0 || (!config.disable_mipmaps && ((width >= 4 && height >= 4) || (format != 0x8C01 && format != 0x8C02))))
    glCompressedTexImage2D(target, level, format, width, height, border, imageSize, data);
}

void glShaderSourceHook(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
  if (!config.use_shader_cache) {
    glShaderSource(shader, count, string, length);
    return;
  }

  uint32_t sha1[5];
  SHA1_CTX ctx;

  sha1_init(&ctx);
  sha1_update(&ctx, (uint8_t *)*string, *length);
  sha1_final(&ctx, (uint8_t *)sha1);

  char path[1024];
  snprintf(path, sizeof(path), "%s/%08x%08x%08x%08x%08x.gxp", SHADER_CACHE_PATH, sha1[0], sha1[1], sha1[2], sha1[3], sha1[4]);

  size_t shaderSize;
  void *shaderBuf;

  FILE *file = sceLibcBridge_fopen(path, "rb");
  if (file) {
    sceLibcBridge_fseek(file, 0, SEEK_END);
    shaderSize = sceLibcBridge_ftell(file);
    sceLibcBridge_fseek(file, 0, SEEK_SET);

    shaderBuf = malloc(shaderSize);
    sceLibcBridge_fread(shaderBuf, 1, shaderSize, file);
    sceLibcBridge_fclose(file);

    glShaderBinary(1, &shader, 0, shaderBuf, shaderSize);

    free(shaderBuf);
  } else {
    GLint type;
    glGetShaderiv(shader, GL_SHADER_TYPE, &type);
    shark_type sharkType = type == GL_FRAGMENT_SHADER ? SHARK_FRAGMENT_SHADER : SHARK_VERTEX_SHADER;

    shaderSize = *length;
    shaderBuf = shark_compile_shader_extended(*string, &shaderSize, sharkType, SHARK_OPT_UNSAFE, SHARK_ENABLE, SHARK_ENABLE, SHARK_ENABLE);
    glShaderBinary(1, &shader, 0, shaderBuf, shaderSize);

    file = sceLibcBridge_fopen(path, "w");
    if (file) {
      sceLibcBridge_fwrite(shaderBuf, 1, shaderSize, file);
      sceLibcBridge_fclose(file);
    }

    shark_clear_output();
  }
}

void glCompileShaderHook(GLuint shader) {
  if (!config.use_shader_cache)
    glCompileShader(shader);
}

extern void *_ZdaPv;
extern void *_ZdlPv;
extern void *_Znaj;
extern void *_Znwj;

extern void *__aeabi_d2ulz;
extern void *__aeabi_idiv;
extern void *__aeabi_idivmod;
extern void *__aeabi_l2d;
extern void *__aeabi_l2f;
extern void *__aeabi_ldivmod;
extern void *__aeabi_uidiv;
extern void *__aeabi_uidivmod;
extern void *__aeabi_ul2d;
extern void *__aeabi_ul2f;
extern void *__aeabi_uldivmod;

// extern void *__assert2;
extern void *__cxa_atexit;
extern void *__cxa_finalize;
extern void *__stack_chk_fail;
extern void *__stack_chk_guard;

int __signbit(double d) {
  return signbit(d);
}

int __isfinite(double d) {
  return isfinite(d);
}

void *sceClibMemclr(void *dst, SceSize len) {
  return sceClibMemset(dst, 0, len);
}

void *sceClibMemset2(void *dst, SceSize len, int ch) {
  return sceClibMemset(dst, ch, len);
}

static int EnterGameFromSCFunc = 0;
static int SigningOutfromApp = 0;
static int hasTouchScreen = 0;

static int __stack_chk_guard_fake = 0x42424242;

static FILE *stderr_fake;
static FILE __sF_fake[0x100][3];

static DynLibFunction dynlib_functions[] = {
  { "_ZdaPv", (uintptr_t)&_ZdaPv },
  { "_ZdlPv", (uintptr_t)&_ZdlPv },
  { "_Znaj", (uintptr_t)&_Znaj },
  { "_Znwj", (uintptr_t)&_Znwj },

  { "__aeabi_d2ulz", (uintptr_t)&__aeabi_d2ulz },
  { "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod },
  { "__aeabi_idiv", (uintptr_t)&__aeabi_idiv },
  { "__aeabi_l2d", (uintptr_t)&__aeabi_l2d },
  { "__aeabi_l2f", (uintptr_t)&__aeabi_l2f },
  { "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod },
  { "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
  { "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
  { "__aeabi_ul2d", (uintptr_t)&__aeabi_ul2d },
  { "__aeabi_ul2f", (uintptr_t)&__aeabi_ul2f },
  { "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod },

  { "__aeabi_memclr", (uintptr_t)&sceClibMemclr },
  { "__aeabi_memclr4", (uintptr_t)&sceClibMemclr },
  { "__aeabi_memclr8", (uintptr_t)&sceClibMemclr },
  { "__aeabi_memcpy", (uintptr_t)&sceClibMemcpy },
  { "__aeabi_memcpy4", (uintptr_t)&sceClibMemcpy },
  { "__aeabi_memcpy8", (uintptr_t)&sceClibMemcpy },
  { "__aeabi_memmove", (uintptr_t)&sceClibMemmove },
  { "__aeabi_memmove4", (uintptr_t)&sceClibMemmove },
  { "__aeabi_memmove8", (uintptr_t)&sceClibMemmove },
  { "__aeabi_memset", (uintptr_t)&sceClibMemset2 },
  { "__aeabi_memset4", (uintptr_t)&sceClibMemset2 },
  { "__aeabi_memset8", (uintptr_t)&sceClibMemset2 },

  { "__android_log_print", (uintptr_t)&__android_log_print },
  // { "__assert2", (uintptr_t)&__assert2 },
  { "__cxa_atexit", (uintptr_t)&__cxa_atexit },
  { "__cxa_finalize", (uintptr_t)&__cxa_finalize },
  { "__errno", (uintptr_t)&__errno },
  { "__isfinite", (uintptr_t)&__isfinite },
  { "__sF", (uintptr_t)&__sF_fake },
  { "__signbit", (uintptr_t)&__signbit },
  { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
  { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard_fake },

  { "AAssetManager_fromJava", (uintptr_t)&ret0 },
  { "AAssetManager_open", (uintptr_t)&ret0 },
  { "AAsset_close", (uintptr_t)&ret0 },
  { "AAsset_getLength", (uintptr_t)&ret0 },
  { "AAsset_getRemainingLength", (uintptr_t)&ret0 },
  { "AAsset_read", (uintptr_t)&ret0 },
  { "AAsset_seek", (uintptr_t)&ret0 },

  { "_Z13SetJNEEnvFuncPFPvvE", (uintptr_t)&ret0 },

  { "EnterGameFromSCFunc", (uintptr_t)&EnterGameFromSCFunc },
  { "SigningOutfromApp", (uintptr_t)&SigningOutfromApp },
  { "hasTouchScreen", (uintptr_t)&hasTouchScreen },
  { "IsProfileStatsBusy", (uintptr_t)&ret1 },
  { "_Z15EnterSocialCLubv", (uintptr_t)&ret0 },
  { "_Z12IsSCSignedInv", (uintptr_t)&ret0 },

  { "pthread_attr_destroy", (uintptr_t)&ret0 },
  { "pthread_cond_init", (uintptr_t)&ret0 },
  { "pthread_create", (uintptr_t)&pthread_create_fake },
  { "pthread_getspecific", (uintptr_t)&ret0 },
  { "pthread_key_create", (uintptr_t)&ret0 },
  { "pthread_mutexattr_init", (uintptr_t)&ret0 },
  { "pthread_mutexattr_settype", (uintptr_t)&ret0 },
  { "pthread_mutexattr_destroy", (uintptr_t)&ret0 },
  { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake },
  { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake },
  { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake },
  { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake },
  { "pthread_setspecific", (uintptr_t)&ret0 },

  { "sem_destroy", (uintptr_t)&sem_destroy_fake },
  // { "sem_getvalue", (uintptr_t)&sem_getvalue },
  { "sem_init", (uintptr_t)&sem_init_fake },
  { "sem_post", (uintptr_t)&sem_post_fake },
  // { "sem_trywait", (uintptr_t)&sem_trywait },
  { "sem_wait", (uintptr_t)&sem_wait_fake },

  { "sigaction", (uintptr_t)&ret0 },
  { "sigemptyset", (uintptr_t)&ret0 },

  { "acosf", (uintptr_t)&acosf },
  { "asinf", (uintptr_t)&asinf },
  { "atan2f", (uintptr_t)&atan2f },
  { "atanf", (uintptr_t)&atanf },
  { "ceilf", (uintptr_t)&ceilf },
  { "cos", (uintptr_t)&cos },
  { "cosf", (uintptr_t)&cosf },
  { "exp2", (uintptr_t)&exp2 },
  { "exp2f", (uintptr_t)&exp2f },
  { "exp", (uintptr_t)&exp },
  { "floor", (uintptr_t)&floor },
  { "floorf", (uintptr_t)&floorf },
  { "fmodf", (uintptr_t)&fmodf },
  { "ldexpf", (uintptr_t)&ldexpf },
  { "log10f", (uintptr_t)&log10f },
  { "log", (uintptr_t)&log },
  { "logf", (uintptr_t)&logf },
  { "modf", (uintptr_t)&modf },
  { "modff", (uintptr_t)&modff },
  { "pow", (uintptr_t)&pow },
  { "powf", (uintptr_t)&powf },
  { "sin", (uintptr_t)&sin },
  { "sinf", (uintptr_t)&sinf },
  { "tan", (uintptr_t)&tan },
  { "tanf", (uintptr_t)&tanf },

  { "atof", (uintptr_t)&atof },
  { "atoi", (uintptr_t)&atoi },

  { "islower", (uintptr_t)&islower },
  { "isprint", (uintptr_t)&isprint },
  { "isspace", (uintptr_t)&isspace },

  { "calloc", (uintptr_t)&calloc },
  { "free", (uintptr_t)&free },
  { "malloc", (uintptr_t)&malloc },
  { "realloc", (uintptr_t)&realloc },

  // { "clock_gettime", (uintptr_t)&clock_gettime },
  { "ctime", (uintptr_t)&ctime },
  { "gettimeofday", (uintptr_t)&gettimeofday },
  { "gmtime", (uintptr_t)&gmtime },
  { "localtime_r", (uintptr_t)&localtime_r },
  { "time", (uintptr_t)&time },

  { "eglGetDisplay", (uintptr_t)&ret0 },
  { "eglGetProcAddress", (uintptr_t)&ret0 },
  { "eglQueryString", (uintptr_t)&ret0 },

  { "abort", (uintptr_t)&abort },
  { "exit", (uintptr_t)&exit },

  { "fclose", (uintptr_t)&sceLibcBridge_fclose },
  // { "fdopen", (uintptr_t)&fdopen },
  // { "fegetround", (uintptr_t)&fegetround },
  { "feof", (uintptr_t)&sceLibcBridge_feof },
  { "ferror", (uintptr_t)&sceLibcBridge_ferror },
  // { "fesetround", (uintptr_t)&fesetround },
  // { "fflush", (uintptr_t)&fflush },
  // { "fgetc", (uintptr_t)&fgetc },
  // { "fgets", (uintptr_t)&fgets },
  { "fopen", (uintptr_t)&sceLibcBridge_fopen },
  // { "fprintf", (uintptr_t)&fprintf },
  // { "fputc", (uintptr_t)&fputc },
  // { "fputs", (uintptr_t)&fputs },
  // { "fputwc", (uintptr_t)&fputwc },
  { "fread", (uintptr_t)&sceLibcBridge_fread },
  { "fseek", (uintptr_t)&sceLibcBridge_fseek },
  { "ftell", (uintptr_t)&sceLibcBridge_ftell },
  { "fwrite", (uintptr_t)&sceLibcBridge_fwrite },

  { "getenv", (uintptr_t)&getenv },
  // { "gettid", (uintptr_t)&gettid },

  { "glActiveTexture", (uintptr_t)&glActiveTexture },
  { "glAttachShader", (uintptr_t)&glAttachShader },
  { "glBindAttribLocation", (uintptr_t)&glBindAttribLocation },
  { "glBindBuffer", (uintptr_t)&glBindBuffer },
  { "glBindFramebuffer", (uintptr_t)&glBindFramebuffer },
  { "glBindRenderbuffer", (uintptr_t)&ret0 },
  { "glBindTexture", (uintptr_t)&glBindTexture },
  { "glBlendFunc", (uintptr_t)&glBlendFunc },
  { "glBlendFuncSeparate", (uintptr_t)&glBlendFuncSeparate },
  { "glBufferData", (uintptr_t)&glBufferData },
  { "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
  { "glClear", (uintptr_t)&glClear },
  { "glClearColor", (uintptr_t)&glClearColor },
  { "glClearDepthf", (uintptr_t)&glClearDepthf },
  { "glClearStencil", (uintptr_t)&glClearStencil },
  { "glCompileShader", (uintptr_t)&glCompileShaderHook },
  { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2DHook },
  { "glCreateProgram", (uintptr_t)&glCreateProgram },
  { "glCreateShader", (uintptr_t)&glCreateShader },
  { "glCullFace", (uintptr_t)&glCullFace },
  { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
  { "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffers },
  { "glDeleteProgram", (uintptr_t)&glDeleteProgram },
  { "glDeleteRenderbuffers", (uintptr_t)&ret0 },
  { "glDeleteShader", (uintptr_t)&glDeleteShader },
  { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
  { "glDepthFunc", (uintptr_t)&glDepthFunc },
  { "glDepthMask", (uintptr_t)&glDepthMask },
  { "glDisable", (uintptr_t)&glDisable },
  { "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
  { "glDrawArrays", (uintptr_t)&glDrawArrays },
  { "glDrawElements", (uintptr_t)&glDrawElements },
  { "glEnable", (uintptr_t)&glEnable },
  { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray },
  { "glFramebufferRenderbuffer", (uintptr_t)&ret0 },
  { "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2D },
  { "glFrontFace", (uintptr_t)&glFrontFace },
  { "glGenBuffers", (uintptr_t)&glGenBuffers },
  { "glGenFramebuffers", (uintptr_t)&glGenFramebuffers },
  { "glGenRenderbuffers", (uintptr_t)&ret0 },
  { "glGenTextures", (uintptr_t)&glGenTextures },
  { "glGetAttribLocation", (uintptr_t)&glGetAttribLocation },
  { "glGetError", (uintptr_t)&glGetError },
  { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
  { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog },
  { "glGetProgramiv", (uintptr_t)&glGetProgramiv },
  { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog },
  { "glGetShaderiv", (uintptr_t)&glGetShaderiv },
  { "glGetString", (uintptr_t)&glGetString },
  { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation },
  { "glHint", (uintptr_t)&glHint },
  { "glLinkProgram", (uintptr_t)&glLinkProgram },
  { "glPolygonOffset", (uintptr_t)&glPolygonOffset },
  { "glReadPixels", (uintptr_t)&glReadPixels },
  { "glRenderbufferStorage", (uintptr_t)&ret0 },
  { "glScissor", (uintptr_t)&glScissor },
  { "glShaderSource", (uintptr_t)&glShaderSourceHook },
  { "glTexImage2D", (uintptr_t)&glTexImage2DHook },
  { "glTexParameterf", (uintptr_t)&glTexParameterf },
  { "glTexParameteri", (uintptr_t)&glTexParameteri },
  { "glUniform1fv", (uintptr_t)&glUniform1fv },
  { "glUniform1i", (uintptr_t)&glUniform1i },
  { "glUniform2fv", (uintptr_t)&glUniform2fv },
  { "glUniform3fv", (uintptr_t)&glUniform3fv },
  { "glUniform4fv", (uintptr_t)&glUniform4fv },
  { "glUniformMatrix3fv", (uintptr_t)&glUniformMatrix3fv },
  { "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv },
  { "glUseProgram", (uintptr_t)&glUseProgram },
  { "glVertexAttrib4fv", (uintptr_t)&glVertexAttrib4fv },
  { "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer },
  { "glViewport", (uintptr_t)&glViewport },

  // { "longjmp", (uintptr_t)&longjmp },
  // { "setjmp", (uintptr_t)&setjmp },

  { "memchr", (uintptr_t)&sceClibMemchr },
  { "memcmp", (uintptr_t)&sceClibMemcmp },

  { "puts", (uintptr_t)&puts },
  { "qsort", (uintptr_t)&qsort },

  // { "raise", (uintptr_t)&raise },
  // { "rewind", (uintptr_t)&rewind },

  { "rand", (uintptr_t)&rand },
  { "srand", (uintptr_t)&srand },

  { "sscanf", (uintptr_t)&sscanf },

  // { "close", (uintptr_t)&close },
  // { "closedir", (uintptr_t)&closedir },
  // { "lseek", (uintptr_t)&lseek },
  { "mkdir", (uintptr_t)&mkdir },
  // { "open", (uintptr_t)&open },
  // { "opendir", (uintptr_t)&opendir },
  // { "read", (uintptr_t)&read },
  // { "readdir", (uintptr_t)&readdir },
  // { "remove", (uintptr_t)&remove },
  { "stat", (uintptr_t)&stat },

  { "stderr", (uintptr_t)&stderr_fake },
  { "strcasecmp", (uintptr_t)&strcasecmp },
  { "strcat", (uintptr_t)&strcat },
  { "strchr", (uintptr_t)&strchr },
  { "strcmp", (uintptr_t)&sceClibStrcmp },
  { "strcpy", (uintptr_t)&strcpy },
  { "strerror", (uintptr_t)&strerror },
  { "strlen", (uintptr_t)&strlen },
  { "strncasecmp", (uintptr_t)&sceClibStrncasecmp },
  { "strncat", (uintptr_t)&sceClibStrncat },
  { "strncmp", (uintptr_t)&sceClibStrncmp },
  { "strncpy", (uintptr_t)&sceClibStrncpy },
  { "strpbrk", (uintptr_t)&strpbrk },
  { "strrchr", (uintptr_t)&sceClibStrrchr },
  { "strstr", (uintptr_t)&sceClibStrstr },
  { "strtof", (uintptr_t)&strtof },
  { "strtok", (uintptr_t)&strtok },
  { "strtol", (uintptr_t)&strtol },
  { "strtoul", (uintptr_t)&strtoul },

  { "toupper", (uintptr_t)&toupper },
  { "vasprintf", (uintptr_t)&vasprintf },

  // { "nanosleep", (uintptr_t)&nanosleep },
  { "usleep", (uintptr_t)&usleep },
};

int check_kubridge(void) {
  int search_unk[2];
  return _vshKernelSearchModuleByName("kubridge", search_unk);
}

int file_exists(const char *path) {
  SceIoStat stat;
  return sceIoGetstat(path, &stat) >= 0;
}

int main(int argc, char *argv[]) {
  // Checking if we want to start the companion app
  sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
  SceAppUtilAppEventParam eventParam;
  sceClibMemset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
  sceAppUtilReceiveAppEvent(&eventParam);
  if (eventParam.type == 0x05) {
    char buffer[2048];
    sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
    if (strstr(buffer, "-config"))
      sceAppMgrLoadExec("app0:/companion.bin", NULL, NULL);
  }

  sceKernelChangeThreadPriority(0, 127);
  sceKernelChangeThreadCpuAffinityMask(0, 0x40000);

  sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &panelInfoFront);
  sceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK, &panelInfoBack);

  scePowerSetArmClockFrequency(444);
  scePowerSetBusClockFrequency(222);
  scePowerSetGpuClockFrequency(222);
  scePowerSetGpuXbarClockFrequency(166);

  sceIoMkdir(SHADER_CACHE_PATH, 0777);
  read_config(CONFIG_PATH);
  read_controller_config(CONTROLLER_CONFIG_PATH);

  if (check_kubridge() < 0)
    fatal_error("Error kubridge.skprx is not installed.");

  if (!file_exists("ur0:/data/libshacccg.suprx"))
    fatal_error("Error libshacccg.suprx is not installed.");

  if (so_load(SO_PATH) < 0)
    fatal_error("Error could not load %s.", SO_PATH);

  stderr_fake = stderr;
  so_relocate();
  so_resolve(dynlib_functions, sizeof(dynlib_functions) / sizeof(DynLibFunction), 1);

  patch_openal();
  patch_opengl();
  patch_game();
  patch_gfx();
  so_flush_caches();

  so_execute_init_array();
  so_free_temp();

  if (fios_init() < 0)
    fatal_error("Error could not initialize fios.");

  vglSetupRuntimeShaderCompiler(SHARK_OPT_UNSAFE, SHARK_ENABLE, SHARK_ENABLE, SHARK_ENABLE);
  vglInitExtended(0, SCREEN_W, SCREEN_H, 24 * 1024 * 1024, config.aa_mode);
  vglUseVram(GL_TRUE);

  jni_load();

  return 0;
}

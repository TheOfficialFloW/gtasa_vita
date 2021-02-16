/* jni_patch.c -- Fake Java Native Interface
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <vitaGL.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "config.h"
#include "so_util.h"

enum MethodIDs {
  UNKNOWN = 0,
  INIT_EGL_AND_GLES2,
  SWAP_BUFFERS,
  MAKE_CURRENT,
  UN_MAKE_CURRENT,
  GET_APP_LOCAL_VALUE,
  FILE_GET_ARCHIVE_NAME,
  DELETE_FILE,
  GET_DEVICE_INFO,
  GET_DEVICE_TYPE,
  GET_DEVICE_LOCALE,
  GET_GAMEPAD_TYPE,
  GET_GAMEPAD_BUTTONS,
  GET_GAMEPAD_AXIS,
} MethodIDs;

typedef struct {
  char *name;
  enum MethodIDs id;
} NameToMethodID;

static NameToMethodID name_to_method_ids[] = {
  { "InitEGLAndGLES2", INIT_EGL_AND_GLES2 },
  { "swapBuffers", SWAP_BUFFERS },
  { "makeCurrent", MAKE_CURRENT },
  { "unMakeCurrent", UN_MAKE_CURRENT },

  { "getAppLocalValue", GET_APP_LOCAL_VALUE },

  { "FileGetArchiveName", FILE_GET_ARCHIVE_NAME },
  { "DeleteFile", DELETE_FILE },

  { "GetDeviceInfo", GET_DEVICE_INFO },
  { "GetDeviceType", GET_DEVICE_TYPE },
  { "GetDeviceLocale", GET_DEVICE_LOCALE },

  { "GetGamepadType", GET_GAMEPAD_TYPE },
  { "GetGamepadButtons", GET_GAMEPAD_BUTTONS },
  { "GetGamepadAxis", GET_GAMEPAD_AXIS },
};

static char fake_vm[0x1000];
static char fake_env[0x1000];
static void *natives;

int GetDeviceInfo(void) {
  return 0;
}

int GetDeviceType(void) {
  // 0x1: phone
  // 0x2: tegra
  // low memory is < 256
  return (MEMORY_MB << 6) | (3 << 2) | 0x1;
}

int GetDeviceLocale(void) {
  return 0; // english
}

int GetGamepadType(void) {
  // 0, 5, 6: XBOX 360
  // 4: MogaPocket
  // 7: MogaPro
  // 8: PS3
  // 9: IOSExtended
  // 10: IOSSimple
  return 8;
}

int GetGamepadButtons(void) {
  int mask = 0;

  SceCtrlData pad;
  sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

  SceTouchData touch;
  sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

  if (pad.buttons & SCE_CTRL_CROSS)
    mask |= 0x1;
  if (pad.buttons & SCE_CTRL_CIRCLE)
    mask |= 0x2;
  if (pad.buttons & SCE_CTRL_SQUARE)
    mask |= 0x4;
  if (pad.buttons & SCE_CTRL_TRIANGLE)
    mask |= 0x8;
  if (pad.buttons & SCE_CTRL_START)
    mask |= 0x10;
  if (pad.buttons & SCE_CTRL_SELECT)
    mask |= 0x20;
  if (pad.buttons & SCE_CTRL_L1)
    mask |= 0x40;
  if (pad.buttons & SCE_CTRL_R1)
    mask |= 0x80;
  if (pad.buttons & SCE_CTRL_UP)
    mask |= 0x100;
  if (pad.buttons & SCE_CTRL_DOWN)
    mask |= 0x200;
  if (pad.buttons & SCE_CTRL_LEFT)
    mask |= 0x400;
  if (pad.buttons & SCE_CTRL_RIGHT)
    mask |= 0x800;
  if (pad.buttons & SCE_CTRL_L3)
    mask |= 0x1000;
  if (pad.buttons & SCE_CTRL_R3)
    mask |= 0x2000;

  for (int i = 0; i < touch.reportNum; i++) {
    for (int i = 0; i < touch.reportNum; i++) {
      if (touch.report[i].y >= (panelInfoFront.minAaY + panelInfoFront.maxAaY) / 2) {
        if (touch.report[i].x < (panelInfoFront.minAaX + panelInfoFront.maxAaX) / 2) {
          if (touch.report[i].x >= config.touch_x_margin)
            mask |= 0x1000; // L3
        } else {
          if (touch.report[i].x < (panelInfoFront.maxAaX - config.touch_x_margin))
            mask |= 0x2000; // R3
        }
      }
    }
  }

  return mask;
}

float GetGamepadAxis(int a0, int axis) {
  SceCtrlData pad;
  sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

  SceTouchData touch;
  sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch, 1);

  float val = 0.0f;

  switch (axis) {
    case 0:
      val = ((float)pad.lx - 128.0f) / 128.0f;
      break;
    case 1:
      val = ((float)pad.ly - 128.0f) / 128.0f;
      break;
    case 2:
      val = ((float)pad.rx - 128.0f) / 128.0f;
      break;
    case 3:
      val = ((float)pad.ry - 128.0f) / 128.0f;
      break;
    case 4: // L2
    case 5: // R2
    {
      if (axis == 4 && pad.buttons & SCE_CTRL_L2) {
        val = 1.0f;
        break;
      } else if (axis == 5 && pad.buttons & SCE_CTRL_R2) {
        val = 1.0f;
        break;
      }

      for (int i = 0; i < touch.reportNum; i++) {
        if (touch.report[i].y < (panelInfoBack.minAaY + panelInfoBack.maxAaY) / 2) {
          if (touch.report[i].x < (panelInfoBack.minAaX + panelInfoBack.maxAaX) / 2) {
            if (touch.report[i].x >= config.touch_x_margin)
              if (axis == 4) val = 1.0f;
          } else {
            if (touch.report[i].x < (panelInfoBack.maxAaX - config.touch_x_margin))
              if (axis == 5) val = 1.0f;
          }
        }
      }
    }
  }

  if (fabsf(val) > 0.25f)
    return val;

  return 0.0f;
}

int swapBuffers(void) {
  vglSwapBuffers(input_cheat ? GL_TRUE : GL_FALSE);
  return 1;
}

int InitEGLAndGLES2(void) {
  vglWaitVblankStart(GL_FALSE);
  return 1;
}

int makeCurrent(void) {
  return 1;
}

int unMakeCurrent(void) {
  return 1;
}

char *getAppLocalValue(char *key) {
  if (strcmp(key, "STORAGE_ROOT") == 0)
    return DATA_PATH;
  return NULL;
}

char *FileGetArchiveName(int type) {
  switch (type) {
    case 1:
      return "/Android/main.obb";
    case 2:
      return "/Android/patch.obb";
    default:
      return NULL;
  }
}

int DeleteFile(char *file) {
  char path[128];
  snprintf(path, sizeof(path), "%s/%s", DATA_PATH, file);
  if (sceIoRemove(path) < 0)
    return 0;
  return 1;
}

int CallBooleanMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case INIT_EGL_AND_GLES2:
      return InitEGLAndGLES2();
    case SWAP_BUFFERS:
      return swapBuffers();
    case MAKE_CURRENT:
      return makeCurrent();
    case UN_MAKE_CURRENT:
      return unMakeCurrent();
    case DELETE_FILE:
      return DeleteFile((char *)args[0]);
    default:
      break;
  }

  return 0;
}

float CallFloatMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_GAMEPAD_AXIS:
      return GetGamepadAxis(args[0], args[1]);
    default:
      break;
  }

  return 0.0f;
}

int CallIntMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_GAMEPAD_TYPE:
      return GetGamepadType();
    case GET_GAMEPAD_BUTTONS:
      return GetGamepadButtons();
    case GET_DEVICE_INFO:
      return GetDeviceInfo();
    case GET_DEVICE_TYPE:
      return GetDeviceType();
    case GET_DEVICE_LOCALE:
      return GetDeviceLocale();
    default:
      break;
  }

  return 0;
}

void *CallObjectMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  switch (methodID) {
    case GET_APP_LOCAL_VALUE:
      return getAppLocalValue((char *)args[0]);
    case FILE_GET_ARCHIVE_NAME:
      return FileGetArchiveName(args[0]);
  }

  return NULL;
}

void CallVoidMethodV(void *env, void *obj, int methodID, uintptr_t *args) {
  return;
}

int GetMethodID(void *env, void *class, const char *name, const char *sig) {
  // debugPrintf("%s\n", name);

  for (int i = 0; i < sizeof(name_to_method_ids) / sizeof(NameToMethodID); i++) {
    if (strcmp(name, name_to_method_ids[i].name) == 0) {
      // debugPrintf("Return ID: %d\n", name_to_method_ids[i].id);
      return name_to_method_ids[i].id;
    }
  }

  return UNKNOWN;
}

void RegisterNatives(void *env, int r1, void *r2) {
  natives = r2;
}

void *NewGlobalRef(void) {
  return (void *)0x42424242;
}

char *NewStringUTF(void *env, char *bytes) {
  return bytes;
}

char *GetStringUTFChars(void *env, char *string, int *isCopy) {
  return string;
}

void *NVThreadGetCurrentJNIEnv(void) {
  return fake_env;
}

int GetEnv(void *vm, void **env, int r2) {
  memset(fake_env, 'A', sizeof(fake_env));
  *(uintptr_t *)(fake_env + 0x00) = (uintptr_t)fake_env; // just point to itself...
  *(uintptr_t *)(fake_env + 0x18) = (uintptr_t)ret0; // FindClass
  *(uintptr_t *)(fake_env + 0x44) = (uintptr_t)ret0; // ExceptionClear
  *(uintptr_t *)(fake_env + 0x54) = (uintptr_t)NewGlobalRef;
  *(uintptr_t *)(fake_env + 0x5C) = (uintptr_t)ret0; // DeleteLocalRef
  *(uintptr_t *)(fake_env + 0x84) = (uintptr_t)GetMethodID;
  *(uintptr_t *)(fake_env + 0x8C) = (uintptr_t)CallObjectMethodV;
  *(uintptr_t *)(fake_env + 0x98) = (uintptr_t)CallBooleanMethodV;
  *(uintptr_t *)(fake_env + 0xC8) = (uintptr_t)CallIntMethodV;
  *(uintptr_t *)(fake_env + 0xE0) = (uintptr_t)CallFloatMethodV;
  *(uintptr_t *)(fake_env + 0xF8) = (uintptr_t)CallVoidMethodV;
  *(uintptr_t *)(fake_env + 0x178) = (uintptr_t)ret0; // NvEventQueueActivity stuff
  *(uintptr_t *)(fake_env + 0x1C4) = (uintptr_t)ret0;
  *(uintptr_t *)(fake_env + 0x1CC) = (uintptr_t)ret0;
  *(uintptr_t *)(fake_env + 0x240) = (uintptr_t)ret0; // keyboard stuff
  *(uintptr_t *)(fake_env + 0x29C) = (uintptr_t)NewStringUTF;
  *(uintptr_t *)(fake_env + 0x2A4) = (uintptr_t)GetStringUTFChars;
  *(uintptr_t *)(fake_env + 0x2A8) = (uintptr_t)ret0; // ReleaseStringUTFChars
  *(uintptr_t *)(fake_env + 0x35C) = (uintptr_t)RegisterNatives;
  *env = fake_env;
  return 0;
}

void jni_load(void) {
  *(int *)so_find_addr("IsAndroidPaused") = 0; // it's 1 by default

  memset(fake_vm, 'A', sizeof(fake_vm));
  *(uintptr_t *)(fake_vm + 0x00) = (uintptr_t)fake_vm; // just point to itself...
  *(uintptr_t *)(fake_vm + 0x18) = (uintptr_t)GetEnv;

  int (* JNI_OnLoad)(void *vm, void *reserved) = (void *)so_find_addr("JNI_OnLoad");
  JNI_OnLoad(fake_vm, NULL);

  int (* init)(void *env, int r1, int init_graphics) = *(void **)(natives + 8);
  init(fake_env, 0, 1);
}
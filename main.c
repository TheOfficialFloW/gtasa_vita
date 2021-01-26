/* main.c -- Grand Theft Auto: San Andreas .so loader
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <psp2/power.h>
#include <psp2/touch.h>
#include <kubridge.h>
#include <vitashark.h>
#include <vitaGL.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <math_neon.h>

#include "main.h"
#include "so_util.h"
#include "openal_patch.h"
#include "opengl_patch.h"
#include "sha1.h"

int _newlib_heap_size_user = MEMORY_MB * 1024 * 1024;

int debugPrintf(char *text, ...) {
  va_list list;
  static char string[0x1000];

  va_start(list, text);
  vsprintf(string, text, list);
  va_end(list);

  SceUID fd = sceIoOpen("ux0:data/gtasa_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
  if (fd >= 0) {
    sceIoWrite(fd, string, strlen(string));
    sceIoClose(fd);
  }

  return 0;
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
  va_list list;
  static char string[0x1000];

  va_start(list, fmt);
  vsprintf(string, fmt, list);
  va_end(list);

  debugPrintf("%s: %s\n", tag, string);

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

char *GetRockstarID(void) {
  return "flow";
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

int GetDeviceType(void) {
  // 0x1: phone
  // 0x2: tegra
  // low memory is < 256
  return (MEMORY_MB << 6) | (3 << 2) | 0x1;
}

int GetDeviceLocale(void) {
  return 0; // english
}

// 0, 5, 6: XBOX 360
// 4: MogaPocket
// 7: MogaPro
// 8: PS3
// 9: IOSExtended
// 10: IOSSimple
int GetGamepadType(void) {
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

  for (int i = 0; i < touch.reportNum; i++) {
    if (touch.report[i].y > 1088/2) {
      if (touch.report[i].x < 1920/2)
        mask |= 0x1000; // L3
      else
        mask |= 0x2000; // R3
    }
  }

  return mask;
}

float GetGamepadAxis(int r0, int axis) {
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
      for (int i = 0; i < touch.reportNum; i++) {
        if (touch.report[i].y < (890+110)/2) {
          if (touch.report[i].x < 1920/2) {
            if (axis == 4)
              val = 1.0f;
          } else {
            if (axis == 5)
              val = 1.0f;
          }
        }
      }
    }
  }

  if (fabsf(val) > 0.25f)
    return val;

  return 0.0f;
}

int ProcessEvents(void) {
  return 0; // 1 is exit!
}

// only used for NVEventAppMain
int pthread_create_fake(int r0, int r1, int r2, void *arg) {
  int (* func)() = *(void **)(arg + 4);
  return func();
}

int pthread_mutex_init_fake(int *uid) {
  *uid = sceKernelCreateMutex("mutex", SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, NULL);
  if (*uid < 0)
    return -1;
  return 0;
}

int pthread_mutex_destroy_fake(int *uid) {
  if (sceKernelDeleteMutex(*uid) < 0)
    return -1;
  return 0;
}

int pthread_mutex_lock_fake(int *uid) {
  if (!*uid)
    pthread_mutex_init_fake(uid);
  if (sceKernelLockMutex(*uid, 1, NULL) < 0)
    return -1;
  return 0;
}

int pthread_mutex_unlock_fake(int *uid) {
  if (sceKernelUnlockMutex(*uid, 1) < 0)
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
  return func(arg);
}

// OS_ThreadLaunch CdStream with priority 6b
// OS_ThreadLaunch Es2Thread with priority 40
// OS_ThreadLaunch MainThread with priority 5a
// OS_ThreadLaunch BankLoader with priority bf
// OS_ThreadLaunch StreamThread with priority 6b
void *OS_ThreadLaunch(int (* func)(), void *arg, int r2, char *name, int r4, int priority) {
  int min_priority = 191;
  int max_priority = 64;
  int vita_priority;

  switch (priority) {
    case 0:
      vita_priority = min_priority;
      break;
    case 1:
      vita_priority = min_priority - 2 * (min_priority - max_priority) / 3;
      break;
    case 2:
      vita_priority = min_priority - 4 * (min_priority - max_priority) / 5;
      break;
    case 3:
      vita_priority = max_priority;
      break;
    default:
      vita_priority = 0x10000100;
      break;
  }

  // debugPrintf("OS_ThreadLaunch %s with priority %x\n", name, vita_priority);

  SceUID thid = sceKernelCreateThread(name, (SceKernelThreadEntry)thread_stub, vita_priority, 1 * 1024 * 1024, 0, 0, NULL);
  if (thid >= 0) {
    char *out = malloc(0x48);

    uintptr_t args[3];
    args[0] = (uintptr_t)func;
    args[1] = (uintptr_t)arg;
    args[2] = (uintptr_t)out;
    sceKernelStartThread(thid, sizeof(args), args);

    return out;
  }

  return NULL;
}

int swapBuffers(void) {
  vglSwapBuffers();
  return 1;
}

int InitEGLAndGLES2(void) {
  vglWaitVblankStart(GL_TRUE);
  return 1; // success
}

char *OS_FileGetArchiveName(int mode) {
  char *out = malloc(1);
  out[0] = '\0';
  return out;
}

void *TouchSense(void *this) {
  return this;
}

enum MethodIDs {
  UNKNOWN = 0,
  INIT_EGL_AND_GLES2,
  FINISH,
  SWAP_BUFFERS,
  MAKE_CURRENT,
  UNMAKE_CURRENT,
  GET_DEVICE_INFO,
  GET_DEVICE_TYPE,
  GET_DEVICE_LOCALE,
  GET_GAMEPAD_TYPE,
  GET_GAMEPAD_BUTTONS,
  GET_GAMEPAD_AXIS,
  GET_GAMEPAD_TRACK,
} MethodIDs;

typedef struct {
  char *name;
  enum MethodIDs id;
} NameToMethodID;

NameToMethodID name_to_method_ids[] = {
  { "InitEGLAndGLES2", INIT_EGL_AND_GLES2 },
  { "swapBuffers", SWAP_BUFFERS },

  { "GetDeviceInfo", GET_DEVICE_INFO },
  { "GetDeviceType", GET_DEVICE_TYPE },
  { "GetDeviceLocale", GET_DEVICE_LOCALE },

  { "GetGamepadType", GET_GAMEPAD_TYPE },
  { "GetGamepadButtons", GET_GAMEPAD_BUTTONS },
  { "GetGamepadAxis", GET_GAMEPAD_AXIS },
};

void *NVThreadGetCurrentJNIEnv(void) {
  return (void *)0x1337;
}

int CallBooleanMethod(void *env, void *obj, int methodID, int a0, int a1, int a2) {
  // debugPrintf("%s\n", __FUNCTION__);
  switch (methodID) {
    case INIT_EGL_AND_GLES2:
      return InitEGLAndGLES2();
    case SWAP_BUFFERS:
      return swapBuffers();
    default:
      break;
  }

  return 0;
}

int CallIntMethod(void *env, void *obj, int methodID, int a0, int a1, int a2) {
  // debugPrintf("%s\n", __FUNCTION__);
  switch (methodID) {
    case GET_GAMEPAD_TYPE:
      return GetGamepadType();
    case GET_GAMEPAD_BUTTONS:
      return GetGamepadButtons();
    case GET_DEVICE_TYPE:
      return GetDeviceType();
    case GET_DEVICE_LOCALE:
      return GetDeviceLocale();
    default:
      break;
  }

  return 0;
}

float CallFloatMethod(void *env, void *obj, int methodID, int a0, int a1, int a2) {
  // debugPrintf("%s\n", __FUNCTION__);
  switch (methodID) {
    case GET_GAMEPAD_AXIS:
      return GetGamepadAxis(a0, a1);
    default:
      break;
  }

  return 0.0f;
}

int GetStaticMethodID(void *env, void *clazz, const char *name, const char *sig) {
  debugPrintf("%s\n", name);

  for (int i = 0; i < sizeof(name_to_method_ids) / sizeof(NameToMethodID); i++) {
    if (strcmp(name, name_to_method_ids[i].name) == 0) {
      debugPrintf("Return ID: %d\n", name_to_method_ids[i].id);
      return name_to_method_ids[i].id;
    }
  }

  return UNKNOWN;
}

static char fake_vm[0x1000];
static char fake_env[0x1000];
static void *natives;

void RegisterNatives(void *env, int r1, void *r2) {
  natives = r2;
}

int some_jni_function() {
  return 0x42424242;
}

int GetEnvFake(void *vm, void **env, int r2) {
  memset(fake_env, 'A', sizeof(fake_env));
  *(uintptr_t *)(fake_env + 0x00) = (uintptr_t)fake_env; // just point to itself...
  *(uintptr_t *)(fake_env + 0x18) = (uintptr_t)ret0;
  *(uintptr_t *)(fake_env + 0x44) = (uintptr_t)ret0; // keyboard stuff
  *(uintptr_t *)(fake_env + 0x54) = (uintptr_t)some_jni_function;
  *(uintptr_t *)(fake_env + 0x84) = (uintptr_t)GetStaticMethodID;
  *(uintptr_t *)(fake_env + 0x178) = (uintptr_t)ret0; // NvEventQueueActivity stuff
  *(uintptr_t *)(fake_env + 0x240) = (uintptr_t)ret0; // keyboard stuff
  *(uintptr_t *)(fake_env + 0x35C) = (uintptr_t)RegisterNatives;
  *env = fake_env;
  return 0;
}

void launch_game(void) {
  strcpy((char *)so_find_addr("StorageRootBuffer"), "ux0:data/gtasa");
  *(int *)so_find_addr("IsAndroidPaused") = 0; // it's 1 by default

  memset(fake_vm, 'A', sizeof(fake_vm));
  *(uintptr_t *)(fake_vm + 0x00) = (uintptr_t)fake_vm; // just point to itself...
  *(uintptr_t *)(fake_vm + 0x18) = (uintptr_t)GetEnvFake;

  int (* JNI_OnLoad)(void *vm, void *reserved) = (void *)so_find_addr("JNI_OnLoad");
  JNI_OnLoad(fake_vm, NULL);

  int (* init)(void *env, int r1, int init_graphics) = *(void **)(natives + 8);
  init(fake_env, 0, 1);
}

void *OS_ThreadSetValue(void *RenderQueue) {
  *(uint8_t *)(RenderQueue + 601) = 0;
  return NULL;
}

extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;

// TODO: be careful of inlining
void patch_game(void) {
  hook_thumb(so_find_addr("__cxa_guard_acquire"), (uintptr_t)&__cxa_guard_acquire);
  hook_thumb(so_find_addr("__cxa_guard_release"), (uintptr_t)&__cxa_guard_release);

  hook_thumb(so_find_addr("_Z24NVThreadGetCurrentJNIEnvv"), (uintptr_t)NVThreadGetCurrentJNIEnv);

  hook_thumb(so_find_addr("_ZN7_JNIEnv17CallBooleanMethodEP8_jobjectP10_jmethodIDz"), (uintptr_t)CallBooleanMethod);
  hook_thumb(so_find_addr("_ZN7_JNIEnv13CallIntMethodEP8_jobjectP10_jmethodIDz"), (uintptr_t)CallIntMethod);
  hook_thumb(so_find_addr("_ZN7_JNIEnv15CallFloatMethodEP8_jobjectP10_jmethodIDz"), (uintptr_t)CallFloatMethod);

  // dummy so we don't crash with NVThreadGetCurrentJNIEnv
  hook_thumb(so_find_addr("_Z10NvUtilInitv"), (uintptr_t)ret0);

  // used for openal
  hook_thumb(so_find_addr("InitializeCriticalSection"), (uintptr_t)ret0);

  // used in NVEventAppMain
  hook_thumb(so_find_addr("_Z21OS_ApplicationPreinitv"), (uintptr_t)ret0);

  // used to check some flags
  hook_thumb(so_find_addr("_Z20OS_ServiceAppCommandPKcS0_"), (uintptr_t)ret0);

  hook_thumb(so_find_addr("_Z15OS_ThreadLaunchPFjPvES_jPKci16OSThreadPriority"), (uintptr_t)OS_ThreadLaunch);

  hook_thumb(so_find_addr("_Z17OS_ScreenGetWidthv"), (uintptr_t)OS_ScreenGetWidth);
  hook_thumb(so_find_addr("_Z18OS_ScreenGetHeightv"), (uintptr_t)OS_ScreenGetHeight);

  // TODO: set deviceChip, definedDevice
  hook_thumb(so_find_addr("_Z20AND_SystemInitializev"), (uintptr_t)ret0);

  // TODO: implement touch here
  hook_thumb(so_find_addr("_Z13ProcessEventsb"), (uintptr_t)ProcessEvents);

  // no obb
  hook_thumb(so_find_addr("_Z22AND_FileGetArchiveName13OSFileArchive"), (uintptr_t)OS_FileGetArchiveName);

  // no cloud
  hook_thumb(so_find_addr("_Z22SCCloudSaveStateUpdatev"), (uintptr_t)ret0);

  // no touchsense
  hook_thumb(so_find_addr("_ZN10TouchSenseC2Ev"), (uintptr_t)TouchSense);

  // no telemetry
  hook_thumb(so_find_addr("_Z11updateUsageb"), (uintptr_t)ret0);

  // do not use mutex for RenderQueue
  hook_thumb(so_find_addr("_Z17OS_ThreadSetValuePv"), (uintptr_t)OS_ThreadSetValue);
}

void glTexImage2DHook(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * data) {
  if (level == 0)
    glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
}

void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
  //debugPrintf("glGetProgramiv pname: 0x%X\n", pname);
  if (pname == GL_INFO_LOG_LENGTH)
    *params = 0;
  else
    *params = GL_TRUE;
}

void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
  if (length) *length = 0;
}

#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
void glGetIntegervHook(GLenum pname, GLint *data) {
  //debugPrintf("glGetIntegerv pname: 0x%X\n", pname);
  glGetIntegerv(pname, data);
  if (pname == GL_MAX_VERTEX_UNIFORM_VECTORS)
    *data = (63 * 3) + 32; // set RQMaxBones=63
  else if (pname == 0x8B82)
    *data = GL_TRUE;
  else if (pname == GL_DRAW_FRAMEBUFFER_BINDING)
    *data = 0;
}

void glShaderSourceHook(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
#ifdef ENABLE_SHADER_CACHE
  if (string) {
    uint32_t sha1[5];
    SHA1_CTX ctx;

    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t *)*string, *length);
    sha1_final(&ctx, (uint8_t *)sha1);

    char path[1024];
    snprintf(path, sizeof(path), "%s/%08x%08x%08x%08x%08x.gxp", SHADER_CACHE_PATH, sha1[0], sha1[1], sha1[2], sha1[3], sha1[4]);

    size_t shaderSize;
    void *shaderBuf;

    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd >= 0) {
      shaderSize = sceIoLseek(fd, 0, SCE_SEEK_END);
      sceIoLseek(fd, 0, SCE_SEEK_SET);

      shaderBuf = malloc(shaderSize);
      sceIoRead(fd, shaderBuf, shaderSize);
      sceIoClose(fd);

      glShaderBinary(1, &shader, 0, shaderBuf, shaderSize);

      free(shaderBuf);
    } else {
      GLint type;
      glGetShaderiv(shader, GL_SHADER_TYPE, &type);
      shark_type sharkType = type == GL_FRAGMENT_SHADER ? SHARK_FRAGMENT_SHADER : SHARK_VERTEX_SHADER;

      shaderSize = *length;
      shaderBuf = shark_compile_shader_extended(*string, &shaderSize, sharkType, SHARK_OPT_UNSAFE, SHARK_ENABLE, SHARK_ENABLE, SHARK_ENABLE);
      if (shaderBuf == NULL)
        debugPrintf("Could not compile shader: %s\n", *string);

      glShaderBinary(1, &shader, 0, shaderBuf, shaderSize);

      fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
      if (fd >= 0) {
        sceIoWrite(fd, shaderBuf, shaderSize);
        sceIoClose(fd);
      }

      shark_clear_output();
    }
    return;
  }
#endif

  glShaderSource(shader, count, string, length);
}

void glCompileShaderHook(GLuint shader) {
#ifndef ENABLE_SHADER_CACHE
  glCompileShader(shader);
#endif
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
extern void *__aeabi_memclr4;
extern void *__aeabi_memclr8;
extern void *__aeabi_memclr;
extern void *__aeabi_memcpy4;
extern void *__aeabi_memcpy8;
extern void *__aeabi_memcpy;
extern void *__aeabi_memmove4;
extern void *__aeabi_memmove8;
extern void *__aeabi_memmove;
extern void *__aeabi_memset4;
extern void *__aeabi_memset8;
extern void *__aeabi_memset;
extern void *__aeabi_uidiv;
extern void *__aeabi_uidivmod;
extern void *__aeabi_ul2d;
extern void *__aeabi_ul2f;
extern void *__aeabi_uldivmod;

// extern void *__assert2;
extern void *__cxa_atexit;
extern void *__cxa_finalize;
// extern void *__errno;
// extern void *__isfinite;
// extern void *__sF;
// extern void *__signbit;
extern void *__stack_chk_fail;
extern void *__stack_chk_guard;

int __signbit(double d) {
  return signbit(d);
}

int __isfinite(double d) {
  return isfinite(d);
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
  { "__aeabi_memclr4", (uintptr_t)&__aeabi_memclr4 },
  { "__aeabi_memclr8", (uintptr_t)&__aeabi_memclr8 },
  { "__aeabi_memclr", (uintptr_t)&__aeabi_memclr },
  { "__aeabi_memcpy4", (uintptr_t)&__aeabi_memcpy4 },
  { "__aeabi_memcpy8", (uintptr_t)&__aeabi_memcpy8 },
  { "__aeabi_memcpy", (uintptr_t)&__aeabi_memcpy },
  { "__aeabi_memmove4", (uintptr_t)&__aeabi_memmove4 },
  { "__aeabi_memmove8", (uintptr_t)&__aeabi_memmove8 },
  { "__aeabi_memmove", (uintptr_t)&__aeabi_memmove },
  { "__aeabi_memset4", (uintptr_t)&__aeabi_memset4 },
  { "__aeabi_memset8", (uintptr_t)&__aeabi_memset8 },
  { "__aeabi_memset", (uintptr_t)&__aeabi_memset },
  { "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
  { "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
  { "__aeabi_ul2d", (uintptr_t)&__aeabi_ul2d },
  { "__aeabi_ul2f", (uintptr_t)&__aeabi_ul2f },
  { "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod },

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

  { "EnterGameFromSCFunc", (uintptr_t)&EnterGameFromSCFunc },
  { "GetRockstarID", (uintptr_t)&GetRockstarID },
  { "SigningOutfromApp", (uintptr_t)&SigningOutfromApp },
  { "hasTouchScreen", (uintptr_t)&hasTouchScreen },
  { "_Z15EnterSocialCLubv", (uintptr_t)ret0 },
  { "_Z12IsSCSignedInv", (uintptr_t)ret0 },

  { "pthread_cond_init", (uintptr_t)&ret0 },
  { "pthread_create", (uintptr_t)&pthread_create_fake },
  { "pthread_getspecific", (uintptr_t)&ret0 },
  { "pthread_key_create", (uintptr_t)&ret0 },
  { "pthread_mutexattr_init", (uintptr_t)&ret0 },
  { "pthread_mutexattr_settype", (uintptr_t)&ret0 },
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

  { "fclose", (uintptr_t)&fclose },
  { "fdopen", (uintptr_t)&fdopen },
  // { "fegetround", (uintptr_t)&fegetround },
  { "feof", (uintptr_t)&feof },
  { "ferror", (uintptr_t)&ferror },
  // { "fesetround", (uintptr_t)&fesetround },
  { "fflush", (uintptr_t)&fflush },
  { "fgetc", (uintptr_t)&fgetc },
  { "fgets", (uintptr_t)&fgets },
  { "fopen", (uintptr_t)&fopen },
  // { "fprintf", (uintptr_t)&fprintf },
  // { "fputc", (uintptr_t)&fputc },
  // { "fputs", (uintptr_t)&fputs },
  // { "fputwc", (uintptr_t)&fputwc },
  { "fread", (uintptr_t)&fread },
  { "fseek", (uintptr_t)&fseek },
  { "ftell", (uintptr_t)&ftell },
  { "fwrite", (uintptr_t)&fwrite },

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
  { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
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
  { "glGetIntegerv", (uintptr_t)&glGetIntegervHook },
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

  { "memchr", (uintptr_t)&memchr },
  { "memcmp", (uintptr_t)&memcmp },

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
  { "stat", (uintptr_t)stat },

  { "stderr", (uintptr_t)&stderr_fake },
  { "strcasecmp", (uintptr_t)&strcasecmp },
  { "strcat", (uintptr_t)&strcat },
  { "strchr", (uintptr_t)&strchr },
  { "strcmp", (uintptr_t)&strcmp },
  { "strcpy", (uintptr_t)&strcpy },
  { "strerror", (uintptr_t)&strerror },
  { "strlen", (uintptr_t)&strlen },
  { "strncasecmp", (uintptr_t)&strncasecmp },
  { "strncat", (uintptr_t)&strncat },
  { "strncmp", (uintptr_t)&strncmp },
  { "strncpy", (uintptr_t)&strncpy },
  { "strpbrk", (uintptr_t)&strpbrk },
  { "strrchr", (uintptr_t)&strrchr },
  { "strstr", (uintptr_t)&strstr },
  { "strtof", (uintptr_t)&strtof },
  { "strtok", (uintptr_t)&strtok },
  { "strtol", (uintptr_t)&strtol },
  { "strtoul", (uintptr_t)&strtoul },

  { "toupper", (uintptr_t)&toupper },
  { "vasprintf", (uintptr_t)&vasprintf },

  // { "nanosleep", (uintptr_t)&nanosleep },
  { "usleep", (uintptr_t)&usleep },
};

int main(int argc, char *argv[]) {
  sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
  sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);

  scePowerSetArmClockFrequency(444);
  scePowerSetBusClockFrequency(222);
  scePowerSetGpuClockFrequency(222);
  scePowerSetGpuXbarClockFrequency(166);

#ifdef ENABLE_SHADER_CACHE
  sceIoMkdir(SHADER_CACHE_PATH, 0777);
#endif

  stderr_fake = stderr;

  so_load(SO_PATH);
  so_resolve(dynlib_functions, sizeof(dynlib_functions) / sizeof(DynLibFunction));

  patch_openal();
  patch_opengl();
  patch_game();
  so_flush_caches();

  so_execute_init_array();
  so_free_temp();

  vglSetupRuntimeShaderCompiler(SHARK_OPT_UNSAFE, SHARK_ENABLE, SHARK_ENABLE, SHARK_ENABLE);
  vglInitExtended(SCREEN_W, SCREEN_H, 0x1000000, SCE_GXM_MULTISAMPLE_4X);
  vglUseVram(GL_TRUE);

  launch_game();

  return 0;
}

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
#include "jni_patch.h"
#include "openal_patch.h"
#include "opengl_patch.h"
#include "gfx_patch.h"
#include "sha1.h"

int _newlib_heap_size_user = MEMORY_MB * 1024 * 1024;

SceTouchPanelInfo panelInfoFront, panelInfoBack;

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

int OS_SystemChip(void) {
  return 19; // default
}

int OS_ScreenGetHeight(void) {
  return SCREEN_H;
}

int OS_ScreenGetWidth(void) {
  return SCREEN_W;
}

int ProcessEvents(void) {
  return 0; // 1 is exit!
}

// only used for NVEventAppMain
int pthread_create_fake(int r0, int r1, int r2, void *arg) {
  int (* func)() = *(void **)(arg + 4);
  return func();
}

int pthread_mutex_init_fake(SceKernelLwMutexWork **work) {
  *work = (SceKernelLwMutexWork *)malloc(sizeof(SceKernelLwMutexWork));
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

// CdStream with priority 0
// Es2Thread with priority 0
// MainThread with priority 1
// StreamThread with priority 2
// BankLoader with priority 3
void *OS_ThreadLaunch(int (* func)(), void *arg, int r2, char *name, int r4, int priority) {
  int min_priority = 0xA0;
  int max_priority = 0x60;
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

  SceUID thid = sceKernelCreateThread(name, (SceKernelThreadEntry)thread_stub, vita_priority, 128 * 1024, 0, 0, NULL);
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

extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;

void patch_game(void) {
  *(int *)so_find_addr("UseCloudSaves") = 0;
  *(int *)so_find_addr("UseTouchSense") = 0;
#ifdef DISABLE_DETAIL_TEXTURES
  *(int *)so_find_addr("gNoDetailTextures") = 1;
#endif

#ifdef FIX_SKIN_WEIGHTS
  // Force using GL_UNSIGNED_SHORT
  uint16_t movs_r1_1 = 0x2101;
  kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x001C8064), &movs_r1_1, sizeof(movs_r1_1));
  kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x001C8082), &movs_r1_1, sizeof(movs_r1_1));
#endif

  hook_thumb(so_find_addr("__cxa_guard_acquire"), (uintptr_t)&__cxa_guard_acquire);
  hook_thumb(so_find_addr("__cxa_guard_release"), (uintptr_t)&__cxa_guard_release);

  hook_thumb(so_find_addr("_Z24NVThreadGetCurrentJNIEnvv"), (uintptr_t)NVThreadGetCurrentJNIEnv);

  // used for openal
  hook_thumb(so_find_addr("InitializeCriticalSection"), (uintptr_t)ret0);

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
}

void glTexImage2DHook(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * data) {
  if (level == 0)
    glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
}

void glCompressedTexImage2DHook(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data) {
  // mips for PVRTC textures break when they're under 1 block in size
  if (level == 0 || (width >= 4 && height >= 4) || (format != 0x8C01 && format != 0x8C02))
    glCompressedTexImage2D(target, level, format, width, height, border, imageSize, data);
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
  sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &panelInfoFront);
  sceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK, &panelInfoBack);

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
#ifdef ENABLE_SKYGFX
  patch_gfx();
#endif
  so_flush_caches();

  so_execute_init_array();
  so_free_temp();

  vglSetupRuntimeShaderCompiler(SHARK_OPT_UNSAFE, SHARK_ENABLE, SHARK_ENABLE, SHARK_ENABLE);
  vglInitExtended(SCREEN_W, SCREEN_H, 0x1000000, SCE_GXM_MULTISAMPLE_4X);
  vglUseVram(GL_TRUE);

  jni_load();

  return 0;
}

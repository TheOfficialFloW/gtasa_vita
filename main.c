/*
  TODO:
  - Fix street lamps
  - Fix compressed textures
  - Fix mip map
  - Implement touch
  - Use math neon
  - Use 4th core
*/

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <psp2/power.h>
#include <psp2/touch.h>
#include <kubridge.h>
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

#include "elf.h"

#include "config.h"

#define MEMORY_MB 272

int _newlib_heap_size_user = MEMORY_MB * 1024 * 1024;

void *memcpy_neon(void *destination, const void *source, size_t num);

void openal_patch();
void opengl_patch();

void *text_base, *data_base;
uint32_t text_size, data_size;

static Elf32_Sym *syms;
static int n_syms;

static char *dynstrtab;

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

int load_file(char *filename, void **data) {
  SceUID fd;
  SceUID blockid;
  size_t size;

  fd = sceIoOpen(filename, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);

  blockid = sceKernelAllocMemBlock("file", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, (size + 0xfff) & ~0xfff, NULL);
  if (blockid < 0)
    return blockid;

  sceKernelGetMemBlockBase(blockid, data);

  sceIoRead(fd, *data, size);
  sceIoClose(fd);

  return blockid;
}

uintptr_t find_addr_by_symbol(char *symbol) {
  for (int i = 0; i < n_syms; i++) {
    char *name = dynstrtab + syms[i].st_name;
    if (strcmp(name, symbol) == 0)
      return (uintptr_t)text_base + syms[i].st_value;
  }

  debugPrintf("Could not find symbol %s\n", symbol);
  return 0;
}

void hook_thumb(uintptr_t addr, uintptr_t dst) {
  if (addr == 0)
    return;
  addr &= ~1;
  if (addr & 2) {
    uint16_t nop = 0xbf00;
    kuKernelCpuUnrestrictedMemcpy((void *)addr, &nop, sizeof(nop));
    addr += 2;
  }
  uint32_t hook[2];
  hook[0] = 0xf000f8df; // LDR PC, [PC]
  hook[1] = dst;
  kuKernelCpuUnrestrictedMemcpy((void *)addr, hook, sizeof(hook));
}

void hook_arm(uintptr_t addr, uintptr_t dst) {
  if (addr == 0)
    return;
  uint32_t hook[2];
  hook[0] = 0xe51ff004; // LDR PC, [PC, #-0x4]
  hook[1] = dst;
  kuKernelCpuUnrestrictedMemcpy((void *)addr, hook, sizeof(hook));
}

// Only used in ReadALConfig
char *getenv(const char *name) {
  return NULL;
}

void __aeabi_atexit() {
  return;
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

int ret0() {
  return 0;
}

int ret1() {
  return 1;
}

int NvAPKOpen(char *path) {
  // debugPrintf("NvAPKOpen: %s\n", path);
  return 0;
}

int mkdir(const char *pathname, mode_t mode) {
  // debugPrintf("mkdir: %s\n", pathname);
  if (sceIoMkdir(pathname, mode) < 0)
    return -1;
  return 0;
}

char *GetRockstarID() {
  return "flow";
}

int OS_SystemChip() {
  return 0;
}

int AND_DeviceType() {
  // 0x1: phone
  // 0x2: tegra
  // low memory is < 256
  return (MEMORY_MB << 6) | (3 << 2) | 0x1;
}

int AND_DeviceLocale() {
  return 0; // english
}

int OS_ScreenGetHeight() {
  return 544;
}

int OS_ScreenGetWidth() {
  return 960;
}

// 0, 5, 6: XBOX 360
// 4: MogaPocket
// 7: MogaPro
// 8: PS3
// 9: IOSExtended
// 10: IOSSimple
int WarGamepad_GetGamepadType() {
  return 8;
}

int WarGamepad_GetGamepadButtons() {
  int mask = 0;

  SceCtrlData pad;
  sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

  SceTouchData touch;
  sceTouchPeek(0, &touch, 1);

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

float WarGamepad_GetGamepadAxis(int axis) {
  SceCtrlData pad;
  sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

  SceTouchData touch;
  sceTouchPeek(0, &touch, 1);

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
        if (touch.report[i].y < 1088/2) {
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

  if (fabsf(val) > 0.2f)
    return val;

  return 0.0f;
}

int ProcessEvents() {
  return 0; // 1 is exit!
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

int thread_stub(SceSize args, int *argp) {
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

void NVEventEGLSwapBuffers() {
  vglStopRendering(GL_TRUE);
  glFinish();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  vglStartRendering();
}

void NVEventEGLMakeCurrent() {
}

void NVEventEGLUnmakeCurrent() {
}

int NVEventEGLInit(void) {
  debugPrintf("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));

  vglWaitVblankStart(GL_TRUE);
  return 1; // success
}

#define APK_PATH "main.obb"

char *OS_FileGetArchiveName(int mode) {
  char *out = malloc(strlen(APK_PATH) + 1);
  out[0] = '\0';
  if (mode == 1) // main
    strcpy(out, APK_PATH);
  return out;
}

FILE *fopen_hook(const char *filename, const char *mode) {
  FILE *file = fopen(filename, mode);
  if (!file)
    debugPrintf("fopen failed for %s\n", filename);
  return file;
}

void
matmul4_neon(float m0[16], float m1[16], float d[16])
{
  asm volatile (
    "vld1.32  {d0, d1},   [%1]!\n\t" //q0 = m1
    "vld1.32  {d2, d3},   [%1]!\n\t" //q1 = m1+4
    "vld1.32  {d4, d5},   [%1]!\n\t" //q2 = m1+8
    "vld1.32  {d6, d7},   [%1]\n\t"  //q3 = m1+12
    "vld1.32  {d16, d17}, [%0]!\n\t" //q8 = m0
    "vld1.32  {d18, d19}, [%0]!\n\t" //q9 = m0+4
    "vld1.32  {d20, d21}, [%0]!\n\t" //q10 = m0+8
    "vld1.32  {d22, d23}, [%0]\n\t"  //q11 = m0+12

    "vmul.f32 q12, q8,  d0[0]\n\t"   //q12 = q8 * d0[0]
    "vmul.f32 q13, q8,  d2[0]\n\t"   //q13 = q8 * d2[0]
    "vmul.f32 q14, q8,  d4[0]\n\t"   //q14 = q8 * d4[0]
    "vmul.f32 q15, q8,  d6[0]\n\t"   //q15 = q8 * d6[0]
    "vmla.f32 q12, q9,  d0[1]\n\t"   //q12 = q9 * d0[1]
    "vmla.f32 q13, q9,  d2[1]\n\t"   //q13 = q9 * d2[1]
    "vmla.f32 q14, q9,  d4[1]\n\t"   //q14 = q9 * d4[1]
    "vmla.f32 q15, q9,  d6[1]\n\t"   //q15 = q9 * d6[1]
    "vmla.f32 q12, q10, d1[0]\n\t"   //q12 = q10 * d0[0]
    "vmla.f32 q13, q10, d3[0]\n\t"   //q13 = q10 * d2[0]
    "vmla.f32 q14, q10, d5[0]\n\t"   //q14 = q10 * d4[0]
    "vmla.f32 q15, q10, d7[0]\n\t"   //q15 = q10 * d6[0]
    "vmla.f32 q12, q11, d1[1]\n\t"   //q12 = q11 * d0[1]
    "vmla.f32 q13, q11, d3[1]\n\t"   //q13 = q11 * d2[1]
    "vmla.f32 q14, q11, d5[1]\n\t"   //q14 = q11 * d4[1]
    "vmla.f32 q15, q11, d7[1]\n\t"   //q15 = q11 * d6[1]

    "vst1.32  {d24, d25}, [%2]!\n\t"  //d = q12
    "vst1.32  {d26, d27}, [%2]!\n\t"  //d+4 = q13
    "vst1.32  {d28, d29}, [%2]!\n\t"  //d+8 = q14
    "vst1.32  {d30, d31}, [%2]\n\t"   //d+12 = q15

    : "+r"(m0), "+r"(m1), "+r"(d) :
    : "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
    "memory"
  );
}

void *(* GetCurrentProjectionMatrix)();

void SetMatrixConstant(void *ES2Shader, int MatrixConstantID, float *matrix) {
#ifdef MVP_OPTIMIZATION
  if (MatrixConstantID == 0) { // Projection matrix
    void *MvpMatrix = ES2Shader + 0x4C * 0;
    float *MvpMatrixData = MvpMatrix + 0x2AC;
    float *MvMatrixData = (ES2Shader + 0x4C * 1) + 0x2AC;
    matmul4_neon(matrix, MvMatrixData, MvpMatrixData);
    *(uint8_t *)(MvpMatrix + 0x2EC) = 1;
    *(uint8_t *)(MvpMatrix + 0x2A8) = 1;
    return;
  } else if (MatrixConstantID == 1) { // Model view matrix
    float *ProjMatrix = (float *)GetCurrentProjectionMatrix();
    // There will be no fresher ProjMatrix, so we should update MvpMatrix as well.
    if (((uint8_t *)ProjMatrix)[64] == 0) {
      void *MvpMatrix = ES2Shader + 0x4C * 0;
      float *MvpMatrixData = MvpMatrix + 0x2AC;
      matmul4_neon(ProjMatrix, matrix, MvpMatrixData);
      *(uint8_t *)(MvpMatrix + 0x2EC) = 1;
      *(uint8_t *)(MvpMatrix + 0x2A8) = 1;
    }
  }
#endif

  void *UniformMatrix = ES2Shader + 0x4C * MatrixConstantID;
  float *UniformMatrixData = UniformMatrix + 0x2AC;

  // That check is so useless IMO. If you need to go through both matrices anways, why just don't copy.
  // if (memcmp(UniformMatrixData, matrix, 16 * 4) != 0) {
    memcpy_neon(UniformMatrixData, matrix, 16 * 4);
    *(uint8_t *)(UniformMatrix + 0x2EC) = 1;
    *(uint8_t *)(UniformMatrix + 0x2A8) = 1;
  // }
}

void functions_patch() {
  // used for openal
  hook_thumb(find_addr_by_symbol("InitializeCriticalSection"), (uintptr_t)ret0);

  // used in NVEventAppMain
  hook_thumb(find_addr_by_symbol("_Z21OS_ApplicationPreinitv"), (uintptr_t)ret0);

  // used to check some flags
  hook_thumb(find_addr_by_symbol("_Z20OS_ServiceAppCommandPKcS0_"), (uintptr_t)ret0);

  hook_thumb(find_addr_by_symbol("_Z15OS_ThreadLaunchPFjPvES_jPKci16OSThreadPriority"), (uintptr_t)OS_ThreadLaunch);

  // egl
  hook_thumb(find_addr_by_symbol("_Z14NVEventEGLInitv"), (uintptr_t)NVEventEGLInit);
  hook_thumb(find_addr_by_symbol("_Z21NVEventEGLMakeCurrentv"), (uintptr_t)NVEventEGLMakeCurrent);
  hook_thumb(find_addr_by_symbol("_Z23NVEventEGLUnmakeCurrentv"), (uintptr_t)NVEventEGLUnmakeCurrent);
  hook_thumb(find_addr_by_symbol("_Z21NVEventEGLSwapBuffersv"), (uintptr_t)NVEventEGLSwapBuffers);

  hook_thumb(find_addr_by_symbol("_Z17OS_ScreenGetWidthv"), (uintptr_t)OS_ScreenGetWidth);
  hook_thumb(find_addr_by_symbol("_Z18OS_ScreenGetHeightv"), (uintptr_t)OS_ScreenGetHeight);

  hook_thumb(find_addr_by_symbol("_Z13OS_SystemChipv"), (uintptr_t)OS_SystemChip);

  hook_thumb(find_addr_by_symbol("_Z14AND_DeviceTypev"), (uintptr_t)AND_DeviceType);
  hook_thumb(find_addr_by_symbol("_Z16AND_DeviceLocalev"), (uintptr_t)AND_DeviceLocale);

  // TODO: set deviceChip, definedDevice
  hook_thumb(find_addr_by_symbol("_Z20AND_SystemInitializev"), (uintptr_t)ret0);

  // TODO: implement touch here
  hook_thumb(find_addr_by_symbol("_Z13ProcessEventsb"), (uintptr_t)ProcessEvents);

  hook_thumb(find_addr_by_symbol("_Z25WarGamepad_GetGamepadTypev"), (uintptr_t)WarGamepad_GetGamepadType);
  hook_thumb(find_addr_by_symbol("_Z28WarGamepad_GetGamepadButtonsv"), (uintptr_t)WarGamepad_GetGamepadButtons);
  hook_thumb(find_addr_by_symbol("_Z25WarGamepad_GetGamepadAxisi"), (uintptr_t)WarGamepad_GetGamepadAxis);

  // oh this is used for obb files!
  // let's extract files, so it's faster
  hook_thumb(find_addr_by_symbol("_Z22AND_FileGetArchiveName13OSFileArchive"), (uintptr_t)OS_FileGetArchiveName);

  // this is for the apk file!
  hook_thumb(find_addr_by_symbol("_Z9NvAPKOpenPKc"), (uintptr_t)ret0);

  // no cloud
  hook_thumb(find_addr_by_symbol("_Z22SCCloudSaveStateUpdatev"), (uintptr_t)ret0);

  // no touchsense
  hook_thumb(find_addr_by_symbol("_ZN10TouchSenseC2Ev"), (uintptr_t)ret0);

  // no telemetry check
  // this is triggered after 10s of no input btw
  hook_thumb(find_addr_by_symbol("_Z11updateUsageb"), (uintptr_t)ret0);

  // hook_thumb(find_addr_by_symbol("_Z24NVThreadGetCurrentJNIEnvv"), (uintptr_t)0x1337);

  // do not check result of CFileMgr::OpenFile in CWaterLevel::WaterLevelInitialise
  uint32_t nop = 0xbf00bf00;
  kuKernelCpuUnrestrictedMemcpy(text_base + 0x004D7A2A, &nop, 2);

#ifdef MVP_OPTIMIZATION
  GetCurrentProjectionMatrix = (void *)find_addr_by_symbol("_Z26GetCurrentProjectionMatrixv");
  hook_thumb(find_addr_by_symbol("_ZN9ES2Shader17SetMatrixConstantE24RQShaderMatrixConstantIDPKf"), (uintptr_t)SetMatrixConstant);
#endif

  // uint16_t bkpt = 0xbe00;
  // kuKernelCpuUnrestrictedMemcpy(text_base + 0x00194968, &bkpt, 2);
}

extern int _Znwj;
extern int _ZdlPv;
extern int _Znaj;
extern int _ZdaPv;

// extern int __aeabi_atexit;

extern int __aeabi_dcmplt;
extern int __aeabi_dmul;
extern int __aeabi_dsub;
extern int __aeabi_idiv;
extern int __aeabi_idivmod;
extern int __aeabi_l2d;
extern int __aeabi_l2f;
extern int __aeabi_ldivmod;
extern int __aeabi_ui2d;
extern int __aeabi_uidiv;
extern int __aeabi_uidivmod;
extern int __aeabi_ul2d;
extern int __aeabi_ul2f;
extern int __aeabi_uldivmod;

// extern int __assert2;
// extern int __errno;

extern int __isfinitef;

extern int __stack_chk_fail;
extern int __stack_chk_guard;

static const short _C_toupper_[] = {
  -1,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z',  0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

const short *_toupper_tab_ = _C_toupper_;

int EnterGameFromSCFunc = 0;
int SigningOutfromApp = 0;

int __stack_chk_guard_fake = 0x42424242;

void glCompressedTexImage2DHook(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data) {
	if (!level) {
		glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
	}
}

static GLint tex_to_kill;
static GLboolean kill_rendering;
void glTexImage2DHook(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * data) {
	if (!level) {
		if (width == 960 && height == 544) glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex_to_kill);
		else glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
	}
}

void glBindTextureHook(GLenum target, GLuint texture) {
	kill_rendering = tex_to_kill == texture;
	glBindTexture(target, texture);
}

void glDrawArraysHook(GLenum mode, GLint first, GLsizei count) {
	if (!kill_rendering) glDrawArrays(mode, first, count);
}

void glHintHook(GLenum target, GLenum mode) {
	glHint(target, GL_FASTEST);
}

void glFramebufferTexture2DHook(GLenum target, GLenum attachment, GLenum textarget, GLuint tex_id, GLint level) {
	if (attachment == GL_COLOR_ATTACHMENT0) {
		glFramebufferTexture2D(target, attachment, textarget, tex_id, level);
	}
}

void glDeleteFramebuffersHook(GLsizei n, GLuint *framebuffers) {}

void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
	//debugPrintf("glGetProgramiv pname: 0x%X\n", pname);
	if (pname == GL_INFO_LOG_LENGTH)
		*params = 0;
	else
		*params = GL_TRUE;
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {}
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {}
void glGenRenderbuffers(GLsizei n, GLuint * renderbuffers) {}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
	if (length) *length = 0;
}

#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
void glGetIntegervHook(GLenum pname, GLint *data) {
  //debugPrintf("glGetIntegerv pname: 0x%X\n", pname);
  glGetIntegerv(pname, data);
  if (pname == GL_MAX_VERTEX_UNIFORM_VECTORS)
    *data = (63 * 3) + 32; // piglet hardcodes 128! this sets RQMaxBones=63
  else if (pname == 0x8B82)
    *data = GL_TRUE;
}

typedef struct {
  char *symbol;
  uintptr_t func;
} DynLibFunction;

DynLibFunction dynlib_functions[] = {
  { "_Znwj", (uintptr_t)&_Znwj },
  { "_ZdlPv", (uintptr_t)&_ZdlPv },
  { "_Znaj", (uintptr_t)&_Znaj },
  { "_ZdaPv", (uintptr_t)&_ZdaPv },

  { "_toupper_tab_", (uintptr_t)&_toupper_tab_ },

  { "EnterGameFromSCFunc", (uintptr_t)&EnterGameFromSCFunc },
  { "SigningOutfromApp", (uintptr_t)&SigningOutfromApp },
  { "_Z15EnterSocialCLubv", (uintptr_t)ret0 },
  { "_Z12IsSCSignedInv", (uintptr_t)ret0 },

  // Not sure how important this is. Used in some init_array.
  { "pthread_key_create", (uintptr_t)&ret0 },

  { "pthread_getspecific", (uintptr_t)&ret0 },
  { "pthread_setspecific", (uintptr_t)&ret0 },

  { "pthread_mutexattr_init", (uintptr_t)&ret0 },
  { "pthread_mutexattr_settype", (uintptr_t)&ret0 },
  { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake },
  { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake },
  { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake },
  { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake },

  { "sem_destroy", (uintptr_t)&sem_destroy_fake },
  // { "sem_getvalue", (uintptr_t)&sem_getvalue },
  { "sem_init", (uintptr_t)&sem_init_fake },
  { "sem_post", (uintptr_t)&sem_post_fake },
  // { "sem_trywait", (uintptr_t)&sem_trywait },
  { "sem_wait", (uintptr_t)&sem_wait_fake },

  { "_Jv_RegisterClasses", (uintptr_t)0 },
  { "_ITM_deregisterTMCloneTable", (uintptr_t)0 },
  { "_ITM_registerTMCloneTable", (uintptr_t)0 },
  { "__deregister_frame_info", (uintptr_t)0 },
  { "__register_frame_info", (uintptr_t)0 },

  { "GetRockstarID", (uintptr_t)&GetRockstarID },

  { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },

  { "__android_log_print", (uintptr_t)__android_log_print },

  // { "__assert2", (uintptr_t)&__assert2 },
  { "__errno", (uintptr_t)&__errno },
  // { "__isfinitef", (uintptr_t)&__isfinitef },

  { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
  // freezes with real __stack_chk_guard
  { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard_fake },

  { "__aeabi_dcmplt", (uintptr_t)&__aeabi_dcmplt },
  { "__aeabi_dmul", (uintptr_t)&__aeabi_dmul },
  { "__aeabi_dsub", (uintptr_t)&__aeabi_dsub },
  { "__aeabi_idiv", (uintptr_t)&__aeabi_idiv },
  { "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod },
  { "__aeabi_l2d", (uintptr_t)&__aeabi_l2d },
  { "__aeabi_l2f", (uintptr_t)&__aeabi_l2f },
  { "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod },
  { "__aeabi_ui2d", (uintptr_t)&__aeabi_ui2d },
  { "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
  { "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
  { "__aeabi_ul2d", (uintptr_t)&__aeabi_ul2d },
  { "__aeabi_ul2f", (uintptr_t)&__aeabi_ul2f },
  { "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod },

   // TODO: use math neon?
  { "acos", (uintptr_t)&acos },
  { "acosf", (uintptr_t)&acosf },
  { "asinf", (uintptr_t)&asinf },
  { "atan2", (uintptr_t)&atan2 },
  { "atan2f", (uintptr_t)&atan2f },
  { "atanf", (uintptr_t)&atanf },
  { "ceilf", (uintptr_t)&ceilf },
  { "cos", (uintptr_t)&cos },
  { "cosf", (uintptr_t)&cosf },
  { "exp", (uintptr_t)&exp },
  { "floor", (uintptr_t)&floor },
  { "floorf", (uintptr_t)&floorf },
  { "fmod", (uintptr_t)&fmod },
  { "fmodf", (uintptr_t)&fmodf },
  { "log", (uintptr_t)&log },
  { "log10f", (uintptr_t)&log10f },
  { "modff", (uintptr_t)&modff },
  { "pow", (uintptr_t)&pow },
  { "powf", (uintptr_t)&powf },
  { "sin", (uintptr_t)&sin },
  { "sinf", (uintptr_t)&sinf },
  { "tan", (uintptr_t)&tan },
  { "tanf", (uintptr_t)&tanf },

  { "atoi", (uintptr_t)&atoi },
  { "isspace", (uintptr_t)&isspace },

  { "calloc", (uintptr_t)&calloc },
  { "free", (uintptr_t)&free },
  { "malloc", (uintptr_t)&malloc },
  { "realloc", (uintptr_t)&realloc },

  // { "clock_gettime", (uintptr_t)&clock_gettime },
  { "ctime", (uintptr_t)&ctime },
  { "gettimeofday", (uintptr_t)&gettimeofday },
  { "gmtime", (uintptr_t)&gmtime },
  { "time", (uintptr_t)&time },

  // { "eglGetDisplay", (uintptr_t)&eglGetDisplay },
  // { "eglGetProcAddress", (uintptr_t)&eglGetProcAddress },
  // { "eglQueryString", (uintptr_t)&eglQueryString },

  { "abort", (uintptr_t)&abort },
  { "exit", (uintptr_t)&exit },

  { "fclose", (uintptr_t)&fclose },
  { "fdopen", (uintptr_t)&fdopen },
  { "fflush", (uintptr_t)&fflush },
  { "fgetc", (uintptr_t)&fgetc },
  { "fgets", (uintptr_t)&fgets },

  { "fopen", (uintptr_t)&fopen },
  // { "fprintf", (uintptr_t)&fprintf },
  // { "fputc", (uintptr_t)&fputc },
  // { "fputs", (uintptr_t)&fputs },
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
  { "glBindRenderbuffer", (uintptr_t)&glBindRenderbuffer },
  { "glBindTexture", (uintptr_t)&glBindTextureHook },
  { "glBlendFunc", (uintptr_t)&glBlendFunc },
  { "glBlendFuncSeparate", (uintptr_t)&glBlendFuncSeparate },
  { "glBufferData", (uintptr_t)&glBufferData },
  { "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
  { "glClear", (uintptr_t)&glClear },
  { "glClearColor", (uintptr_t)&glClearColor },
  { "glClearDepthf", (uintptr_t)&glClearDepthf },
  { "glClearStencil", (uintptr_t)&glClearStencil },
  { "glCompileShader", (uintptr_t)&glCompileShader },
  { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2DHook },
  { "glCreateProgram", (uintptr_t)&glCreateProgram },
  { "glCreateShader", (uintptr_t)&glCreateShader },
  { "glCullFace", (uintptr_t)&glCullFace },
  { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
  { "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffersHook },
  { "glDeleteProgram", (uintptr_t)&glDeleteProgram },
  { "glDeleteRenderbuffers", (uintptr_t)&glDeleteRenderbuffers },
  { "glDeleteShader", (uintptr_t)&glDeleteShader },
  { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
  { "glDepthFunc", (uintptr_t)&glDepthFunc },
  { "glDepthMask", (uintptr_t)&glDepthMask },
  { "glDisable", (uintptr_t)&glDisable },
  { "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
  { "glDrawArrays", (uintptr_t)&glDrawArraysHook },
  { "glDrawElements", (uintptr_t)&glDrawElements },
  { "glEnable", (uintptr_t)&glEnable },
  { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray },
  { "glFramebufferRenderbuffer", (uintptr_t)&glFramebufferRenderbuffer },
  { "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2DHook },
  { "glFrontFace", (uintptr_t)&glFrontFace },
  { "glGenBuffers", (uintptr_t)&glGenBuffers },
  { "glGenFramebuffers", (uintptr_t)&glGenFramebuffers },
  { "glGenRenderbuffers", (uintptr_t)&glGenRenderbuffers },
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
  { "glHint", (uintptr_t)&glHintHook },
  { "glLinkProgram", (uintptr_t)&glLinkProgram },
  { "glPolygonOffset", (uintptr_t)&glPolygonOffset },
  { "glReadPixels", (uintptr_t)&glReadPixels },
  { "glRenderbufferStorage", (uintptr_t)&glRenderbufferStorage },
  { "glScissor", (uintptr_t)&glScissor },
  { "glShaderSource", (uintptr_t)&glShaderSource },
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

  // TODO: check if they are compatible
  // { "longjmp", (uintptr_t)&longjmp },
  // { "setjmp", (uintptr_t)&setjmp },

  { "memcmp", (uintptr_t)&memcmp },
  { "memcpy", (uintptr_t)&memcpy_neon },
  { "memmove", (uintptr_t)&memmove },
  { "memset", (uintptr_t)&memset },

  { "printf", (uintptr_t)&debugPrintf },
  // { "puts", (uintptr_t)&puts },

  { "qsort", (uintptr_t)&qsort },

  // { "raise", (uintptr_t)&raise },
  // { "rewind", (uintptr_t)&rewind },

  // { "scmainUpdate", (uintptr_t)&scmainUpdate },
  // { "slCreateEngine", (uintptr_t)&slCreateEngine },

  { "lrand48", (uintptr_t)&lrand48 },
  { "srand48", (uintptr_t)&srand48 },

  { "snprintf", (uintptr_t)&snprintf },
  { "sprintf", (uintptr_t)&sprintf },
  { "vsnprintf", (uintptr_t)&vsnprintf },
  { "vsprintf", (uintptr_t)&vsprintf },

  { "sscanf", (uintptr_t)&sscanf },

  // { "close", (uintptr_t)&close },
  // { "closedir", (uintptr_t)&closedir },
  // { "lseek", (uintptr_t)&lseek },
  { "mkdir", (uintptr_t)&mkdir },
  // { "open", (uintptr_t)&open },
  // { "opendir", (uintptr_t)&opendir },
  // { "read", (uintptr_t)&read },
  // { "readdir", (uintptr_t)&readdir },
  { "stat", (uintptr_t)stat },
  // { "write", (uintptr_t)&write },

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
  { "strtod", (uintptr_t)&strtod },
  { "strtok", (uintptr_t)&strtok },
  { "strtol", (uintptr_t)&strtol },
  { "strtoul", (uintptr_t)&strtoul },

  // { "syscall", (uintptr_t)&syscall },
  // { "sysconf", (uintptr_t)&sysconf },

  // { "nanosleep", (uintptr_t)&nanosleep },
  { "usleep", (uintptr_t)&usleep },
};

#define ALIGN_MEM(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

int main() {
  sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
  scePowerSetArmClockFrequency(444);
  scePowerSetBusClockFrequency(222);
  scePowerSetGpuClockFrequency(222);
  scePowerSetGpuXbarClockFrequency(166);

  void *so_data, *prog_data;
  SceUID so_blockid, prog_blockid;

  so_blockid = load_file("ux0:data/gtasa/libGTASA.so", &so_data);

  Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)so_data;
  Elf32_Phdr *prog_hdrs = (Elf32_Phdr *)((uintptr_t)so_data + elf_hdr->e_phoff);
  Elf32_Shdr *sec_hdrs = (Elf32_Shdr *)((uintptr_t)so_data + elf_hdr->e_shoff);

  prog_data = (void *)0x98000000;

  for (int i = 0; i < elf_hdr->e_phnum; i++) {
    if (prog_hdrs[i].p_type == PT_LOAD) {
      uint32_t prog_size = ALIGN_MEM(prog_hdrs[i].p_memsz, prog_hdrs[i].p_align);

      if ((prog_hdrs[i].p_flags & PF_X) == PF_X) {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
        opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
        opt.attr = 0x1;
        opt.field_C = (SceUInt32)(prog_data);
        prog_blockid = kuKernelAllocMemBlock("rx_block", SCE_KERNEL_MEMBLOCK_TYPE_USER_RX, prog_size, &opt);

        sceKernelGetMemBlockBase(prog_blockid, &prog_data);

        prog_hdrs[i].p_vaddr += (Elf32_Addr)prog_data;

        text_base = (void *)prog_hdrs[i].p_vaddr;
        text_size = prog_size;
      } else {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
        opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
        opt.attr = 0x1;
        opt.field_C = (SceUInt32)(text_base + text_size);
        prog_blockid = kuKernelAllocMemBlock("rw_block", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, prog_size, &opt);

        sceKernelGetMemBlockBase(prog_blockid, &prog_data);

        prog_hdrs[i].p_vaddr += (Elf32_Addr)text_base;

        data_base = (void *)prog_hdrs[i].p_vaddr;
        data_size = prog_size;
      }

      char *zero = malloc(prog_size);
      memset(zero, 0, prog_size);
      kuKernelCpuUnrestrictedMemcpy(prog_data, zero, prog_size);
      free(zero);

      kuKernelCpuUnrestrictedMemcpy((void *)prog_hdrs[i].p_vaddr, (void *)((uintptr_t)so_data + prog_hdrs[i].p_offset), prog_hdrs[i].p_filesz);
    }
  }

  char *shstrtab = (char *)((uintptr_t)so_data + sec_hdrs[elf_hdr->e_shstrndx].sh_offset);

  int dynsyn_idx = -1;

  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdrs[i].sh_name;

    if (strcmp(sh_name, ".dynsym") == 0) {
      dynsyn_idx = i;
      syms = (Elf32_Sym *)((uintptr_t)text_base + sec_hdrs[dynsyn_idx].sh_addr);
      n_syms = sec_hdrs[dynsyn_idx].sh_size / sizeof(Elf32_Sym);
    } else if (strcmp(sh_name, ".dynstr") == 0) {
      dynstrtab = (char *)((uintptr_t)text_base + sec_hdrs[i].sh_addr);
    } else if (strcmp(sh_name, ".rel.dyn") == 0 || strcmp(sh_name, ".rel.plt") == 0) {
      Elf32_Rel *rels = (Elf32_Rel *)((uintptr_t)text_base + sec_hdrs[i].sh_addr);
      int n_rels = sec_hdrs[i].sh_size / sizeof(Elf32_Rel);

      for (int j = 0; j < n_rels; j++) {
        uint32_t *ptr = (uint32_t *)(text_base + rels[j].r_offset);
        int sym_idx = ELF32_R_SYM(rels[j].r_info);
        Elf32_Sym *sym = &syms[sym_idx];

        switch (ELF32_R_TYPE(rels[j].r_info)) {
          case R_ARM_ABS32:
          {
            *ptr = (uintptr_t)text_base + sym->st_value;
            break;
          }

          case R_ARM_RELATIVE:
          {
            *ptr += (uintptr_t)text_base;
            break;
          }

          case R_ARM_GLOB_DAT:
          case R_ARM_JUMP_SLOT:
          {
            if (sym->st_shndx != SHN_UNDEF) {
              *ptr = (uintptr_t)text_base + sym->st_value;
              break;
            }

            // make it crash for debugging
            *ptr = rels[j].r_offset;

            char *name = dynstrtab + sym->st_name;

            for (int k = 0; k < sizeof(dynlib_functions) / sizeof(DynLibFunction); k++) {
              if (strcmp(name, dynlib_functions[k].symbol) == 0) {
                *ptr = dynlib_functions[k].func;
                break;
              }
            }

            break;
          }

          default:
            debugPrintf("Unknown relocation type: %x\n", ELF32_R_TYPE(rels[j].r_info));
            break;
        }
      }
    }
  }

  vglInitExtended(960, 544, 0x200000, SCE_GXM_MULTISAMPLE_4X);
  vglUseVram(GL_TRUE);
  vglStartRendering();

  openal_patch();
  opengl_patch();
  functions_patch();

  kuFlushIcache(text_base, text_size);

  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdrs[i].sh_name;

    if (strcmp(sh_name, ".init_array") == 0) {
      int (** init_array)() = (void *)((uintptr_t)text_base + sec_hdrs[i].sh_addr);
      int n_array = sec_hdrs[i].sh_size / 4;
      for (int j = 0; j < n_array; j++) {
        if (init_array[j] != 0)
          init_array[j]();
      }
    }
  }

  sceKernelFreeMemBlock(so_blockid);

  strcpy((char *)find_addr_by_symbol("StorageRootBuffer"), "ux0:data/gtasa");
  *(uint32_t *)find_addr_by_symbol("IsAndroidPaused") = 0; // it's 1 by default
  *(uint32_t *)find_addr_by_symbol("DoInitGraphics") = 1;

  int (* NVEventAppMain)(int argc, char *argv[]) = (void *)find_addr_by_symbol("_Z14NVEventAppMainiPPc");
  NVEventAppMain(0, NULL);

  return 0;
}
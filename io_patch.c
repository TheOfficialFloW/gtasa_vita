/* io_patch.c -- use FIOS2 for optimized I/O
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "config.h"
#include "so_util.h"
#include "io_patch.h"

#define MAX_PATH_LENGTH 256

int64_t g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
int64_t g_ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(1024) / sizeof(int64_t) + 1];
int64_t g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(512, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
int64_t g_DHStorage[SCE_FIOS_DH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];

SceFiosRamCacheContext g_RamCacheContext = SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER;
char *g_RamCacheWorkBuffer;

int fios_init(void) {
  int res;

  SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
  params.opStorage.pPtr = g_OpStorage;
  params.opStorage.length = sizeof(g_OpStorage);
  params.chunkStorage.pPtr = g_ChunkStorage;
  params.chunkStorage.length = sizeof(g_ChunkStorage);
  params.fhStorage.pPtr = g_FHStorage;
  params.fhStorage.length = sizeof(g_FHStorage);
  params.dhStorage.pPtr = g_DHStorage;
  params.dhStorage.length = sizeof(g_DHStorage);
  params.pathMax = MAX_PATH_LENGTH;

  res = sceFiosInitialize(&params);
  if (res < 0)
    return res;

  g_RamCacheWorkBuffer = memalign(8, config.io_cache_block_num * config.io_cache_block_size);
  if (!g_RamCacheWorkBuffer)
    return -1;

  g_RamCacheContext.pWorkBuffer = g_RamCacheWorkBuffer;
  g_RamCacheContext.workBufferSize = config.io_cache_block_num * config.io_cache_block_size;
  g_RamCacheContext.blockSize = config.io_cache_block_size;
  res = sceFiosIOFilterAdd(0, sceFiosIOFilterCache, &g_RamCacheContext);
  if (res < 0)
    return res;

  return 0;
}

void fios_terminate(void) {
  sceFiosTerminate();
  free(g_RamCacheWorkBuffer);
}

int fios_open(const char *file, int flags) {
  int res;
  SceFiosFH handle = 0;

  SceFiosOpenParams params = SCE_FIOS_OPENPARAMS_INITIALIZER;
  params.openFlags = flags;
  res = sceFiosFHOpenSync(NULL, &handle, file, &params);
  if (res != SCE_FIOS_OK) {
    debugPrintf("fios_open %s failed: 0x%08X\n", file, res);
    return res;
  }

  return handle;
}

int fios_seek(SceFiosFH handle, SceFiosOffset offset, SceFiosWhence whence) {
  return sceFiosFHSeek(handle, offset, whence);
}

int fios_tell(SceFiosFH handle) {
  return sceFiosFHTell(handle);
}

int fios_getsize(SceFiosFH handle) {
  return sceFiosFHGetSize(handle);
}

int fios_read(SceFiosFH handle, void *data, int size) {
  SceFiosOpAttr attr = SCE_FIOS_OPATTR_INITIALIZER;

  attr.deadline = SCE_FIOS_TIME_EARLIEST;
  attr.priority = SCE_FIOS_PRIO_MAX;

  return sceFiosFHReadSync(&attr, handle, data, (SceFiosSize)size);
}

int fios_write(SceFiosFH handle, const void *data, int size) {
  return sceFiosFHWriteSync(NULL, handle, data, (SceFiosSize)size);
}

int fios_close(SceFiosFH handle) {
  return sceFiosFHCloseSync(NULL, handle);
}

void trim(char *path) {
  for (int i = strlen(path) - 1; i >= 0; i--) {
    if (path[i] == ' ')
      path[i] = '\0';
    else
      break;
  }
}

int OS_FileGetDate(int area, char const *file) {
  return 0;
}

int OS_FileDelete(int area, char const *file) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s", DATA_PATH, file);
  trim(path);
  sceFiosDeleteSync(NULL, path);
  return 0;
}

int OS_FileOpen(int area, void **handle, char const *file, int access) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s", DATA_PATH, file);
  trim(path);
  // debugPrintf("OS_FileOpen: %s, %x, %x\n", path, area, access);
  int flags;
  switch (access) {
    case 0:
      flags = SCE_FIOS_O_RDONLY;
      break;
    case 1:
      flags = SCE_FIOS_O_CREAT | SCE_FIOS_O_WRONLY;
      break;
    case 2:
      flags = SCE_FIOS_O_CREAT | SCE_FIOS_O_RDWR;
      break;
    default:
      debugPrintf("Error unknown access mode %x\n", access);
      return 1;
  }
  *handle = (void *)fios_open(path, flags);
  if ((int)*handle < 0)
    return 1;
  return 0;
}

int OS_FileRead(void *handle, void *data, int size) {
  int read = fios_read((SceFiosFH)handle, data, size);
  if (read < 0)
    return 3;
  if (read != size)
    return 2;
  return 0;
}

int OS_FileWrite(void *handle, void const *data, int size) {
  if (fios_write((SceFiosFH)handle, data, size) != size)
    return 3;
  return 0;
}

int OS_FileGetPosition(void *handle) {
  return fios_tell((SceFiosFH)handle);
}

int OS_FileSetPosition(void *handle, int pos) {
  if (fios_seek((SceFiosFH)handle, pos, SCE_FIOS_SEEK_SET) != pos)
    return 3;
  return 0;
}

int OS_FileSize(void *handle) {
  return fios_getsize((SceFiosFH)handle);
}

int OS_FileClose(void *handle) {
  if (fios_close((SceFiosFH)handle) < 0)
    return 1;
  return 0;
}

void patch_io(void) {
  hook_thumb(so_find_addr("_Z14OS_FileGetDate14OSFileDataAreaPKc"), (uintptr_t)&OS_FileGetDate);
  hook_thumb(so_find_addr("_Z13OS_FileDelete14OSFileDataAreaPKc"), (uintptr_t)&OS_FileDelete);
  hook_thumb(so_find_addr("_Z11OS_FileOpen14OSFileDataAreaPPvPKc16OSFileAccessType"), (uintptr_t)&OS_FileOpen);
  hook_thumb(so_find_addr("_Z11OS_FileReadPvS_i"), (uintptr_t)&OS_FileRead);
  hook_thumb(so_find_addr("_Z12OS_FileWritePvPKvi"), (uintptr_t)&OS_FileWrite);
  hook_thumb(so_find_addr("_Z18OS_FileGetPositionPv"), (uintptr_t)&OS_FileGetPosition);
  hook_thumb(so_find_addr("_Z18OS_FileSetPositionPvi"), (uintptr_t)&OS_FileSetPosition);
  hook_thumb(so_find_addr("_Z11OS_FileSizePv"), (uintptr_t)&OS_FileSize);
  hook_thumb(so_find_addr("_Z12OS_FileClosePv"), (uintptr_t)&OS_FileClose);
}
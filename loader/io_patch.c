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

static int64_t g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(1024) / sizeof(int64_t) + 1];
static int64_t g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(512, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_DHStorage[SCE_FIOS_DH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];

static SceFiosRamCacheContext g_RamCacheContext = SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER;
static char *g_RamCacheWorkBuffer;

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

  params.threadAffinity[SCE_FIOS_IO_THREAD] = 0x20000;
  params.threadAffinity[SCE_FIOS_CALLBACK_THREAD] = 0;
  params.threadAffinity[SCE_FIOS_DECOMPRESSOR_THREAD] = 0;

  params.threadPriority[SCE_FIOS_IO_THREAD] = 64;
  params.threadPriority[SCE_FIOS_CALLBACK_THREAD] = 191;
  params.threadPriority[SCE_FIOS_DECOMPRESSOR_THREAD] = 191;

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
  if (res != SCE_FIOS_OK)
    return res;

  return handle;
}

int64_t fios_seek(SceFiosFH handle, SceFiosOffset offset, SceFiosWhence whence) {
  return sceFiosFHSeek(handle, offset, whence);
}

int64_t fios_tell(SceFiosFH handle) {
  return sceFiosFHTell(handle);
}

int64_t fios_getsize(SceFiosFH handle) {
  return sceFiosFHGetSize(handle);
}

int64_t fios_read(SceFiosFH handle, void *data, size_t size) {
  return sceFiosFHReadSync(NULL, handle, data, size);
}

int64_t fios_write(SceFiosFH handle, const void *data, size_t size) {
  return sceFiosFHWriteSync(NULL, handle, data, size);
}

int fios_close(SceFiosFH handle) {
  return sceFiosFHCloseSync(NULL, handle);
}

typedef struct {
  int handle;
  int eof;
  int error;
} FIOS2_FILE;

FILE *fopen_hook(const char *filename, const char *mode) {
  int flags = 0;
  if (strcmp(mode, "rb") == 0)
    flags = SCE_FIOS_O_RDONLY;
  else if (strcmp(mode, "rb+") == 0)
    flags = SCE_FIOS_O_RDWR;
  else if (strcmp(mode, "wb") == 0)
    flags = SCE_FIOS_O_WRONLY | SCE_FIOS_O_CREAT | SCE_FIOS_O_TRUNC;
  else if (strcmp(mode, "wb+") == 0)
    flags = SCE_FIOS_O_RDWR | SCE_FIOS_O_CREAT | SCE_FIOS_O_TRUNC;

  int handle = fios_open(filename, flags);
  if (handle < 0)
    return NULL;

  FIOS2_FILE *file = malloc(sizeof(FIOS2_FILE));
  file->handle = handle;
  file->eof = 0;
  file->error = 0;

  return (FILE *)file;
}

size_t fread_hook(void *ptr, size_t size, size_t count, FILE *stream) {
  if (size == 0 || count == 0)
    return 0;

  FIOS2_FILE *file = (FIOS2_FILE *)stream;
  size_t read = fios_read(file->handle, ptr, size * count);
  if (read < 0) {
    file->error = 1;
    return -1;
  }
  if (read != size * count)
    file->eof = 1;
  return read;
}

size_t fwrite_hook(const void *ptr, size_t size, size_t count, FILE *stream) {
  if (size == 0 || count == 0)
    return 0;

  FIOS2_FILE *file = (FIOS2_FILE *)stream;
  size_t written = fios_write(file->handle, ptr, size * count);
  if (written != size * count) {
    file->error = 1;
    return -1;
  }
  return written;
}

int fseek_hook(FILE *stream, long int offset, int origin) {
  FIOS2_FILE *file = (FIOS2_FILE *)stream;
  if (fios_seek(file->handle, offset, origin) < 0) {
    file->error = 1;
    return -1;
  }
  return 0;
}

long int ftell_hook(FILE *stream) {
  FIOS2_FILE *file = (FIOS2_FILE *)stream;
  long int offset = fios_tell(file->handle);
  if (offset < 0) {
    file->error = 1;
    return -1;
  }
  return offset;
}

int feof_hook(FILE *stream) {
  FIOS2_FILE *file = (FIOS2_FILE *)stream;
  return file->eof;
}

int ferror_hook(FILE *stream) {
  FIOS2_FILE *file = (FIOS2_FILE *)stream;
  return file->error;
}

int fclose_hook(FILE *stream) {
  FIOS2_FILE *file = (FIOS2_FILE *)stream;
  int handle = file->handle;
  free(file);
  if (fios_close(handle) < 0)
    return EOF;
  return 0;
}

static DynLibFunction dynlib_io_functions[] = {
  { "fclose", (uintptr_t)&fclose_hook },
  { "feof", (uintptr_t)&feof_hook },
  { "ferror", (uintptr_t)&ferror_hook },
  { "fopen", (uintptr_t)&fopen_hook },
  { "fread", (uintptr_t)&fread_hook },
  { "fseek", (uintptr_t)&fseek_hook },
  { "ftell", (uintptr_t)&ftell_hook },
  { "fwrite", (uintptr_t)&fwrite_hook },
};

void patch_io(void) {
  so_resolve(dynlib_io_functions, sizeof(dynlib_io_functions) / sizeof(DynLibFunction), 0);
}
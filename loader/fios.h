#ifndef __FIOS_H__
#define __FIOS_H__

#define SCE_FIOS_FH_SIZE 80
#define SCE_FIOS_DH_SIZE 80
#define SCE_FIOS_OP_SIZE 168
#define SCE_FIOS_CHUNK_SIZE 64

#define SCE_FIOS_ALIGN_UP(val, align) (((val) + ((align) - 1)) & ~((align) - 1))
#define SCE_FIOS_STORAGE_SIZE(num, size) (((num) * (size)) + SCE_FIOS_ALIGN_UP(SCE_FIOS_ALIGN_UP((num), 8) / 8, 8))

#define SCE_FIOS_DH_STORAGE_SIZE(numDHs, pathMax) SCE_FIOS_STORAGE_SIZE(numDHs, SCE_FIOS_DH_SIZE + pathMax)
#define SCE_FIOS_FH_STORAGE_SIZE(numFHs, pathMax) SCE_FIOS_STORAGE_SIZE(numFHs, SCE_FIOS_FH_SIZE + pathMax)
#define SCE_FIOS_OP_STORAGE_SIZE(numOps, pathMax) SCE_FIOS_STORAGE_SIZE(numOps, SCE_FIOS_OP_SIZE + pathMax)
#define SCE_FIOS_CHUNK_STORAGE_SIZE(numChunks) SCE_FIOS_STORAGE_SIZE(numChunks, SCE_FIOS_CHUNK_SIZE)

#define SCE_FIOS_BUFFER_INITIALIZER  { 0, 0 }
#define SCE_FIOS_PARAMS_INITIALIZER { 0, sizeof(SceFiosParams), 0, 0, 2, 1, 0, 0, 256 * 1024, 2, 0, 0, 0, 0, 0, SCE_FIOS_BUFFER_INITIALIZER, SCE_FIOS_BUFFER_INITIALIZER, SCE_FIOS_BUFFER_INITIALIZER, SCE_FIOS_BUFFER_INITIALIZER, NULL, NULL, NULL, { 66, 189, 66 }, { 0x40000, 0, 0x40000}, { 8 * 1024, 16 * 1024, 8 * 1024}}
#define SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER { sizeof(SceFiosRamCacheContext), 0, (64 * 1024), NULL, NULL, 0, {0, 0, 0} }

typedef enum SceFiosThreadType {
  SCE_FIOS_IO_THREAD = 0,
  SCE_FIOS_DECOMPRESSOR_THREAD = 1,
  SCE_FIOS_CALLBACK_THREAD = 2,
  SCE_FIOS_THREAD_TYPES = 3
} SceFiosThreadType;

typedef struct SceFiosRamCacheContext {
  size_t sizeOfContext;
  size_t workBufferSize;
  size_t blockSize;
  void *pWorkBuffer;
  const char *pPath;
  intptr_t flags;
  intptr_t reserved[3];
} SceFiosRamCacheContext;

typedef struct SceFiosBuffer {
  void *pPtr;
  size_t length;
} SceFiosBuffer;

typedef struct SceFiosParams {
  uint32_t initialized : 1;
  uint32_t paramsSize : 15;
  uint32_t pathMax : 16;
  uint32_t profiling;
  uint32_t ioThreadCount;
  uint32_t threadsPerScheduler;
  uint32_t extraFlag1 : 1;
  uint32_t extraFlags : 31;
  uint32_t maxChunk;
  uint8_t maxDecompressorThreadCount;
  uint8_t reserved1;
  uint8_t reserved2;
  uint8_t reserved3;
  intptr_t reserved4;
  intptr_t reserved5;
  SceFiosBuffer opStorage;
  SceFiosBuffer fhStorage;
  SceFiosBuffer dhStorage;
  SceFiosBuffer chunkStorage;
  void *pVprintf;
  void *pMemcpy;
  void *pProfileCallback;
  int threadPriority[3];
  int threadAffinity[3];
  int threadStackSize[3];
} SceFiosParams;

int sceFiosInitialize(const SceFiosParams *params);
void sceFiosTerminate();

int sceFiosIOFilterAdd(int index, void *pFilterCallback, void *pFilterContext);
void sceFiosIOFilterCache();

int fios_init(void);

#endif

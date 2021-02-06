#ifndef __IO_PATCH_H__
#define __IO_PATCH_H__

#define SCE_FIOS_OK 0

#define SCE_FIOS_TIME_NULL ((SceFiosTime)0)
#define SCE_FIOS_TIME_EARLIEST ((SceFiosTime)1)
#define SCE_FIOS_TIME_LATEST ((SceFiosTime)0x7FFFFFFFFFFFFFFFLL)

#define SCE_FIOS_PRIO_MIN ((int8_t)-128)
#define SCE_FIOS_PRIO_DEFAULT ((int8_t)0)
#define SCE_FIOS_PRIO_MAX ((int8_t)127)

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
#define SCE_FIOS_OPENPARAMS_INITIALIZER { 0, 0, 0, SCE_FIOS_BUFFER_INITIALIZER }
#define SCE_FIOS_OPATTR_INITIALIZER { 0, 0, 0, 0, 0, 0, 0, 0 }
#define SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER { sizeof(SceFiosRamCacheContext), 0, (64 * 1024), NULL, NULL, 0, {0, 0, 0} }

typedef enum SceFiosWhence {
  SCE_FIOS_SEEK_SET = 0,
  SCE_FIOS_SEEK_CUR = 1,
  SCE_FIOS_SEEK_END = 2
} SceFiosWhence;

typedef enum SceFiosOpenFlags {
  SCE_FIOS_O_RDONLY = (1 << 0),
  SCE_FIOS_O_WRONLY = (1 << 1),
  SCE_FIOS_O_RDWR = (SCE_FIOS_O_RDONLY | SCE_FIOS_O_WRONLY),
  SCE_FIOS_O_APPEND = (1 << 2),
  SCE_FIOS_O_CREAT = (1 << 3),
  SCE_FIOS_O_TRUNC = (1 << 4),
} SceFiosOpenFlags;

typedef int64_t SceFiosTime;

typedef int32_t SceFiosFH;
typedef int32_t SceFiosDH;
typedef uint64_t SceFiosDate;
typedef int64_t SceFiosOffset;
typedef int64_t SceFiosSize;

typedef struct SceFiosBuffer {
  void *pPtr;
  size_t length;
} SceFiosBuffer;

typedef struct SceFiosOpAttr {
  SceFiosTime deadline;
  void *pCallback;
  void *pCallbackContext;
  int32_t priority : 8;
  uint32_t opflags : 24;
  uint32_t userTag;
  void *userPtr;
  void *pReserved;
} SceFiosOpAttr;

typedef struct SceFiosRamCacheContext {
  size_t sizeOfContext;
  size_t workBufferSize;
  size_t blockSize;
  void *pWorkBuffer;
  const char *pPath;
  intptr_t flags;
  intptr_t reserved[3];
} SceFiosRamCacheContext;

typedef struct SceFiosOpenParams {
  uint32_t openFlags : 16;
  uint32_t opFlags : 16;
  uint32_t reserved;
  SceFiosBuffer buffer;
} SceFiosOpenParams;

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
  void *pVdebugPrintf;
  void *pMemcpy;
  void *pProfileCallback;
  int threadPriority[3];
  int threadAffinity[3];
  int threadStackSize[3];
} SceFiosParams;

typedef struct SceFiosStat {
  SceFiosOffset fileSize;
  SceFiosDate accessDate;
  SceFiosDate modificationDate;
  SceFiosDate creationDate;
  uint32_t statFlags;
  uint32_t reserved;
  int64_t uid;
  int64_t gid;
  int64_t dev;
  int64_t ino;
  int64_t mode;
} SceFiosStat;

int sceFiosInitialize(const SceFiosParams *params);
void sceFiosTerminate();

int sceFiosFileDelete(const SceFiosOpAttr *attr, const char *path);

int sceFiosFHOpenSync(const SceFiosOpAttr *attr, SceFiosFH *fh, const char *path, const void *params);
SceFiosSize sceFiosFHReadSync(const SceFiosOpAttr *attr, SceFiosFH fh, void *data, SceFiosSize size);
SceFiosSize sceFiosFHWriteSync(const SceFiosOpAttr *attr, SceFiosFH fh, const void *data, SceFiosSize size);
int sceFiosFHCloseSync(const SceFiosOpAttr *attr, SceFiosFH fh);

SceFiosOffset sceFiosFHSeek(SceFiosFH fh, SceFiosOffset offset, SceFiosWhence whence);
SceFiosOffset sceFiosFHTell(SceFiosFH fh);
SceFiosSize sceFiosFHGetSize(SceFiosFH fh);

int sceFiosIOFilterAdd(int index, void *pFilterCallback, void *pFilterContext);
void sceFiosIOFilterCache();

int fios_init(void);
void patch_io(void);

#endif

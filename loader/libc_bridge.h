#ifndef __SCE_LIBC_BRIDGE_H__
#define __SCE_LIBC_BRIDGE_H__

#include <stdio.h>

extern void *sceLibcBridge__ZdaPv;
extern void *sceLibcBridge__ZdlPv;
extern void *sceLibcBridge__Znaj;
extern void *sceLibcBridge__Znwj;

extern void *sceLibcBridge___cxa_guard_acquire;
extern void *sceLibcBridge___cxa_guard_release;

int sceLibcBridge_rand(void);
void sceLibcBridge_srand(unsigned int seed);

void sceLibcBridge_qsort(void *base, size_t num, size_t size, int (* compar)(const void *,const void *));

int sceLibcBridge_sscanf(const char *s, const char *format, ...);

void *sceLibcBridge_calloc(size_t num, size_t size);
void sceLibcBridge_free(void *ptr);
void *sceLibcBridge_malloc(size_t size);
void *sceLibcBridge_realloc(void *ptr, size_t size);
void *sceLibcBridge_memalign(size_t alignment, size_t size);

FILE *sceLibcBridge_fopen(const char *filename, const char *mode);
int sceLibcBridge_fclose(FILE *stream);
size_t sceLibcBridge_fread(void *ptr, size_t size, size_t count, FILE *stream);
int sceLibcBridge_fseek(FILE *stream, long int offset, int origin);
long int sceLibcBridge_ftell(FILE *stream);
size_t sceLibcBridge_fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int sceLibcBridge_ferror(FILE *stream);
int sceLibcBridge_feof(FILE *stream);

#endif

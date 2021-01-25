#ifndef __SO_UTIL_H__
#define __SO_UTIL_H__

#define ALIGN_MEM(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

typedef struct {
  char *symbol;
  uintptr_t func;
} DynLibFunction;

extern void *text_base, *data_base;
extern size_t text_size, data_size;

void hook_thumb(uintptr_t addr, uintptr_t dst);
void hook_arm(uintptr_t addr, uintptr_t dst);

void so_flush_caches(void);
int so_free_temp(void);
int so_load(const char *filename);
int so_resolve(DynLibFunction *functions, int num_functions);
void so_execute_init_array(void);
uintptr_t so_find_addr(const char *symbol);

#endif

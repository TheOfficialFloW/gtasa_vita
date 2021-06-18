#ifndef __SO_UTIL_H__
#define __SO_UTIL_H__

#include "elf.h"

#define ALIGN_MEM(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

typedef struct {
  uintptr_t text_base, data_base;
  size_t text_size, data_size;

  SceUID text_blockid, data_blockid;
  SceUID temp_blockid;

  Elf32_Ehdr *elf_hdr;
  Elf32_Phdr *prog_hdr;
  Elf32_Shdr *sec_hdr;

  Elf32_Dyn *dynamic;
  Elf32_Sym *dynsym;
  Elf32_Rel *reldyn;
  Elf32_Rel *relplt;

  int (** init_array)(void);

  char *shstr;
  char *dynstr;

  int num_dynamic;
  int num_dynsym;
  int num_reldyn;
  int num_relplt;
  int num_init_array;
} so_module;

typedef struct {
  char *symbol;
  uintptr_t func;
} DynLibFunction;

void hook_thumb(uintptr_t addr, uintptr_t dst);
void hook_arm(uintptr_t addr, uintptr_t dst);

void so_flush_caches(so_module *mod);
int so_free_temp(so_module *mod);
int so_load(so_module *mod, const char *filename);
int so_relocate(so_module *mod);
int so_resolve(so_module *mod, DynLibFunction *funcs, int num_funcs, int taint_missing_imports);
void so_execute_init_array(so_module *mod);
uintptr_t so_find_addr(so_module *mod, const char *symbol);

#endif

/* so_util.c -- utils to load and hook .so modules
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include <kubridge.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "dialog.h"
#include "so_util.h"

static so_module *head = NULL, *tail = NULL;

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

void hook_addr(uintptr_t addr, uintptr_t dst) {
  if (addr == 0)
    return;
  if (addr & 1)
    hook_thumb(addr, dst);
  else
    hook_arm(addr, dst);
}

void so_flush_caches(so_module *mod) {
  kuKernelFlushCaches((void *)mod->text_base, mod->text_size);
}

int so_load(so_module *mod, const char *filename, uintptr_t load_addr) {
  int res = 0;
  uintptr_t data_addr = 0;
  SceUID so_blockid;
  void *so_data;
  size_t so_size;

  memset(mod, 0, sizeof(so_module));

  SceUID fd = sceIoOpen(filename, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  so_size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);

  so_blockid = sceKernelAllocMemBlock("file", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, (so_size + 0xfff) & ~0xfff, NULL);
  if (so_blockid < 0)
    return so_blockid;

  sceKernelGetMemBlockBase(so_blockid, &so_data);

  sceIoRead(fd, so_data, so_size);
  sceIoClose(fd);

  if (memcmp(so_data, ELFMAG, SELFMAG) != 0) {
    res = -1;
    goto err_free_so;
  }

  mod->ehdr = (Elf32_Ehdr *)so_data;
  mod->phdr = (Elf32_Phdr *)((uintptr_t)so_data + mod->ehdr->e_phoff);
  mod->shdr = (Elf32_Shdr *)((uintptr_t)so_data + mod->ehdr->e_shoff);

  mod->shstr = (char *)((uintptr_t)so_data + mod->shdr[mod->ehdr->e_shstrndx].sh_offset);

  for (int i = 0; i < mod->ehdr->e_phnum; i++) {
    if (mod->phdr[i].p_type == PT_LOAD) {
      void *prog_data;
      size_t prog_size;

      if ((mod->phdr[i].p_flags & PF_X) == PF_X) {
        prog_size = ALIGN_MEM(mod->phdr[i].p_memsz, mod->phdr[i].p_align);

        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
        opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
        opt.attr = 0x1;
        opt.field_C = (SceUInt32)load_addr;
        res = mod->text_blockid = kuKernelAllocMemBlock("rx_block", SCE_KERNEL_MEMBLOCK_TYPE_USER_RX, prog_size, &opt);
        if (res < 0)
          goto err_free_so;

        sceKernelGetMemBlockBase(mod->text_blockid, &prog_data);

        mod->phdr[i].p_vaddr += (Elf32_Addr)prog_data;

        mod->text_base = mod->phdr[i].p_vaddr;
        mod->text_size = mod->phdr[i].p_memsz;

        data_addr = (uintptr_t)prog_data + prog_size;
      } else {
        if (data_addr == 0)
          goto err_free_so;

        prog_size = ALIGN_MEM(mod->phdr[i].p_memsz + mod->phdr[i].p_vaddr - (data_addr - mod->text_base), mod->phdr[i].p_align);

        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
        opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
        opt.attr = 0x1;
        opt.field_C = (SceUInt32)data_addr;
        res = mod->data_blockid = kuKernelAllocMemBlock("rw_block", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, prog_size, &opt);
        if (res < 0)
          goto err_free_text;

        sceKernelGetMemBlockBase(mod->data_blockid, &prog_data);

        mod->phdr[i].p_vaddr += (Elf32_Addr)mod->text_base;

        mod->data_base = mod->phdr[i].p_vaddr;
        mod->data_size = mod->phdr[i].p_memsz;
      }

      char *zero = malloc(prog_size);
      memset(zero, 0, prog_size);
      kuKernelCpuUnrestrictedMemcpy(prog_data, zero, prog_size);
      free(zero);

      kuKernelCpuUnrestrictedMemcpy((void *)mod->phdr[i].p_vaddr, (void *)((uintptr_t)so_data + mod->phdr[i].p_offset), mod->phdr[i].p_filesz);
    }
  }

  for (int i = 0; i < mod->ehdr->e_shnum; i++) {
    char *sh_name = mod->shstr + mod->shdr[i].sh_name;
    uintptr_t sh_addr = mod->text_base + mod->shdr[i].sh_addr;
    size_t sh_size = mod->shdr[i].sh_size;
    if (strcmp(sh_name, ".dynamic") == 0) {
      mod->dynamic = (Elf32_Dyn *)sh_addr;
      mod->num_dynamic = sh_size / sizeof(Elf32_Dyn);
    } else if (strcmp(sh_name, ".dynstr") == 0) {
      mod->dynstr = (char *)sh_addr;
    } else if (strcmp(sh_name, ".dynsym") == 0) {
      mod->dynsym = (Elf32_Sym *)sh_addr;
      mod->num_dynsym = sh_size / sizeof(Elf32_Sym);
    } else if (strcmp(sh_name, ".rel.dyn") == 0) {
      mod->reldyn = (Elf32_Rel *)sh_addr;
      mod->num_reldyn = sh_size / sizeof(Elf32_Rel);
    } else if (strcmp(sh_name, ".rel.plt") == 0) {
      mod->relplt = (Elf32_Rel *)sh_addr;
      mod->num_relplt = sh_size / sizeof(Elf32_Rel);
    } else if (strcmp(sh_name, ".init_array") == 0) {
      mod->init_array = (void *)sh_addr;
      mod->num_init_array = sh_size / sizeof(void *);
    } else if (strcmp(sh_name, ".hash") == 0) {
      mod->hash = (void *)sh_addr;
    }
  }

  if (mod->dynamic == NULL ||
      mod->dynstr == NULL ||
      mod->dynsym == NULL ||
      mod->reldyn == NULL ||
      mod->relplt == NULL) {
    res = -2;
    goto err_free_data;
  }

  for (int i = 0; i < mod->num_dynamic; i++) {
    switch (mod->dynamic[i].d_tag) {
      case DT_SONAME:
        mod->soname = mod->dynstr + mod->dynamic[i].d_un.d_ptr;
        break;
      default:
        break;
    }
  }

  sceKernelFreeMemBlock(so_blockid);

  if (!head && !tail) {
    head = mod;
    tail = mod;
  } else {
    tail->next = mod;
    tail = mod;
  }

  return 0;

err_free_data:
  sceKernelFreeMemBlock(mod->data_blockid);
err_free_text:
  sceKernelFreeMemBlock(mod->text_blockid);
err_free_so:
  sceKernelFreeMemBlock(so_blockid);

  return res;
}

int so_relocate(so_module *mod) {
  for (int i = 0; i < mod->num_reldyn + mod->num_relplt; i++) {
    Elf32_Rel *rel = i < mod->num_reldyn ? &mod->reldyn[i] : &mod->relplt[i - mod->num_reldyn];
    Elf32_Sym *sym = &mod->dynsym[ELF32_R_SYM(rel->r_info)];
    uintptr_t *ptr = (uintptr_t *)(mod->text_base + rel->r_offset);

    int type = ELF32_R_TYPE(rel->r_info);
    switch (type) {
      case R_ARM_ABS32:
        if (sym->st_shndx != SHN_UNDEF)
          *ptr += mod->text_base + sym->st_value;
        else
          *ptr = mod->text_base + rel->r_offset; // make it crash for debugging
        break;

      case R_ARM_RELATIVE:
        *ptr += mod->text_base;
        break;

      case R_ARM_GLOB_DAT:
      case R_ARM_JUMP_SLOT:
      {
        if (sym->st_shndx != SHN_UNDEF)
          *ptr = mod->text_base + sym->st_value;
        else
          *ptr = mod->text_base + rel->r_offset; // make it crash for debugging
        break;
      }

      default:
        fatal_error("Error unknown relocation type %x\n", type);
        break;
    }
  }

  return 0;
}

uintptr_t so_resolve_link(so_module *mod, const char *symbol) {
  for (int i = 0; i < mod->num_dynamic; i++) {
    switch (mod->dynamic[i].d_tag) {
      case DT_NEEDED:
      {
        so_module *curr = head;
        while (curr) {
          if (curr != mod && strcmp(curr->soname, mod->dynstr + mod->dynamic[i].d_un.d_ptr) == 0) {
            uintptr_t link = so_symbol(curr, symbol);
            if (link)
              return link;
          }
          curr = curr->next;
        }

        break;
      }
      default:
        break;
    }
  }

  return 0;
}

int so_resolve(so_module *mod, so_default_dynlib *default_dynlib, int size_default_dynlib, int default_dynlib_only) {
  for (int i = 0; i < mod->num_reldyn + mod->num_relplt; i++) {
    Elf32_Rel *rel = i < mod->num_reldyn ? &mod->reldyn[i] : &mod->relplt[i - mod->num_reldyn];
    Elf32_Sym *sym = &mod->dynsym[ELF32_R_SYM(rel->r_info)];
    uintptr_t *ptr = (uintptr_t *)(mod->text_base + rel->r_offset);

    int type = ELF32_R_TYPE(rel->r_info);
    switch (type) {
      case R_ARM_ABS32:
      case R_ARM_GLOB_DAT:
      case R_ARM_JUMP_SLOT:
      {
        if (sym->st_shndx == SHN_UNDEF) {
          int resolved = 0;
          if (!default_dynlib_only) {
            uintptr_t link = so_resolve_link(mod, mod->dynstr + sym->st_name);
            if (link) {
              // debugPrintf("Resolved from dependencies: %s\n", mod->dynstr + sym->st_name);
              *ptr = link;
              resolved = 1;
            }
          }

          for (int j = 0; j < size_default_dynlib / sizeof(so_default_dynlib); j++) {
            if (strcmp(mod->dynstr + sym->st_name, default_dynlib[j].symbol) == 0) {
              if (resolved) {
                // debugPrintf("Overriden: %s\n", mod->dynstr + sym->st_name);
              } else {
                // debugPrintf("Resolved manually: %s\n", mod->dynstr + sym->st_name);
              }
              *ptr = default_dynlib[j].func;
              resolved = 1;
              break;
            }
          }

          if (!resolved) {
            // debugPrintf("Missing: %s\n", mod->dynstr + sym->st_name);
          }
        }

        break;
      }

      default:
        break;
    }
  }

  return 0;
}

void so_initialize(so_module *mod) {
  for (int i = 0; i < mod->num_init_array; i++) {
    if (mod->init_array[i])
      mod->init_array[i]();
  }
}

uint32_t so_hash(const uint8_t *name) {
  uint64_t h = 0, g;
  while (*name) {
    h = (h << 4) + *name++;
    if ((g = (h & 0xf0000000)) != 0)
      h ^= g >> 24;
    h &= 0x0fffffff;
  }
  return h;
}

uintptr_t so_symbol(so_module *mod, const char *symbol) {
  if (mod->hash) {
    uint32_t hash = so_hash((const uint8_t *)symbol);
    uint32_t nbucket = mod->hash[0];
    uint32_t *bucket = &mod->hash[2];
    uint32_t *chain = &bucket[nbucket];
    for (int i = bucket[hash % nbucket]; i; i = chain[i]) {
      if (mod->dynsym[i].st_shndx == SHN_UNDEF)
        continue;
      if (mod->dynsym[i].st_info != SHN_UNDEF && strcmp(mod->dynstr + mod->dynsym[i].st_name, symbol) == 0)
        return mod->text_base + mod->dynsym[i].st_value;
    }
  }

  for (int i = 0; i < mod->num_dynsym; i++) {
    if (mod->dynsym[i].st_shndx == SHN_UNDEF)
      continue;
    if (mod->dynsym[i].st_info != SHN_UNDEF && strcmp(mod->dynstr + mod->dynsym[i].st_name, symbol) == 0)
      return mod->text_base + mod->dynsym[i].st_value;
  }

  return 0;
}

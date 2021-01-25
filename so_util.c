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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "so_util.h"

#include "elf.h"

void *text_base, *data_base;
size_t text_size, data_size;

SceUID text_blockid, data_blockid;
SceUID so_temp_blockid;

static Elf32_Ehdr *elf_hdr;
static Elf32_Phdr *prog_hdr;
static Elf32_Shdr *sec_hdr;
static Elf32_Sym *syms;
static int num_syms;

static char *shstrtab;
static char *dynstrtab;

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

void so_flush_caches(void) {
  kuKernelFlushCaches(text_base, text_size);
}

int so_free_temp(void) {
  return sceKernelFreeMemBlock(so_temp_blockid);
}

int so_load(const char *filename) {
  int res = 0;
  void *data_addr = NULL;
  void *so_data;
  size_t so_size;

  SceUID fd = sceIoOpen(filename, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  so_size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);

  so_temp_blockid = sceKernelAllocMemBlock("file", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, (so_size + 0xfff) & ~0xfff, NULL);
  if (so_temp_blockid < 0)
    return so_temp_blockid;

  sceKernelGetMemBlockBase(so_temp_blockid, &so_data);

  sceIoRead(fd, so_data, so_size);
  sceIoClose(fd);

  if (memcmp(so_data, ELFMAG, SELFMAG) != 0) {
    res = -1;
    goto err_free_so;
  }

  elf_hdr = (Elf32_Ehdr *)so_data;
  prog_hdr = (Elf32_Phdr *)((uintptr_t)so_data + elf_hdr->e_phoff);
  sec_hdr = (Elf32_Shdr *)((uintptr_t)so_data + elf_hdr->e_shoff);

  shstrtab = (char *)((uintptr_t)so_data + sec_hdr[elf_hdr->e_shstrndx].sh_offset);

  for (int i = 0; i < elf_hdr->e_phnum; i++) {
    if (prog_hdr[i].p_type == PT_LOAD) {
      size_t prog_size = ALIGN_MEM(prog_hdr[i].p_memsz, prog_hdr[i].p_align);
      void *prog_data;

      if ((prog_hdr[i].p_flags & PF_X) == PF_X) {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
        opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
#ifdef LOAD_ADDRESS
        opt.attr = 0x1;
        opt.field_C = (SceUInt32)LOAD_ADDRESS;
#endif
        res = text_blockid = kuKernelAllocMemBlock("rx_block", SCE_KERNEL_MEMBLOCK_TYPE_USER_RX, prog_size, &opt);
        if (res < 0)
          goto err_free_so;

        sceKernelGetMemBlockBase(text_blockid, &prog_data);

        prog_hdr[i].p_vaddr += (Elf32_Addr)prog_data;

        text_base = (void *)prog_hdr[i].p_vaddr;
        text_size = prog_hdr[i].p_memsz;

        data_addr = prog_data + prog_size;
      } else {
        if (data_addr == NULL)
          goto err_free_so;

        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
        opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
        opt.attr = 0x1;
        opt.field_C = (SceUInt32)data_addr;
        res = data_blockid = kuKernelAllocMemBlock("rw_block", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, prog_size, &opt);
        if (res < 0)
          goto err_free_text;

        sceKernelGetMemBlockBase(data_blockid, &prog_data);

        prog_hdr[i].p_vaddr += (Elf32_Addr)text_base;

        data_base = (void *)prog_hdr[i].p_vaddr;
        data_size = prog_hdr[i].p_memsz;
      }

      char *zero = malloc(prog_size);
      memset(zero, 0, prog_size);
      kuKernelCpuUnrestrictedMemcpy(prog_data, zero, prog_size);
      free(zero);

      kuKernelCpuUnrestrictedMemcpy((void *)prog_hdr[i].p_vaddr, (void *)((uintptr_t)so_data + prog_hdr[i].p_offset), prog_hdr[i].p_filesz);
    }
  }

  syms = NULL;
  dynstrtab = NULL;

  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".dynsym") == 0) {
      syms = (Elf32_Sym *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
      num_syms = sec_hdr[i].sh_size / sizeof(Elf32_Sym);
    } else if (strcmp(sh_name, ".dynstr") == 0) {
      dynstrtab = (char *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
    }
  }

  if (syms == NULL || dynstrtab == NULL) {
    res = -2;
    goto err_free_data;
  }

  return 0;

err_free_data:
  sceKernelFreeMemBlock(data_blockid);
err_free_text:
  sceKernelFreeMemBlock(text_blockid);
err_free_so:
  sceKernelFreeMemBlock(so_temp_blockid);

  return res;
}

int so_resolve(DynLibFunction *functions, int num_functions) {
  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".rel.dyn") == 0 || strcmp(sh_name, ".rel.plt") == 0) {
      Elf32_Rel *rels = (Elf32_Rel *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
      for (int j = 0; j < sec_hdr[i].sh_size / sizeof(Elf32_Rel); j++) {
        uintptr_t *ptr = (uintptr_t *)(text_base + rels[j].r_offset);
        Elf32_Sym *sym = &syms[ELF32_R_SYM(rels[j].r_info)];

        switch (ELF32_R_TYPE(rels[j].r_info)) {
          case R_ARM_ABS32:
          {
            *ptr += (uintptr_t)text_base + sym->st_value;
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

            for (int k = 0; k < num_functions; k++) {
              if (strcmp(name, functions[k].symbol) == 0) {
                *ptr = functions[k].func;
                break;
              }
            }

            break;
          }

          default:
            debugPrintf("Error unknown relocation type: %x\n", ELF32_R_TYPE(rels[j].r_info));
            break;
        }
      }
    }
  }

  return 0;
}

void so_execute_init_array(void) {
  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".init_array") == 0) {
      int (** init_array)() = (void *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
      for (int j = 0; j < sec_hdr[i].sh_size / 4; j++) {
        if (init_array[j] != 0)
          init_array[j]();
      }
    }
  }
}

uintptr_t so_find_addr(const char *symbol) {
  for (int i = 0; i < num_syms; i++) {
    char *name = dynstrtab + syms[i].st_name;
    if (strcmp(name, symbol) == 0)
      return (uintptr_t)text_base + syms[i].st_value;
  }

  debugPrintf("Error could not find symbol %s\n", symbol);
  return 0;
}

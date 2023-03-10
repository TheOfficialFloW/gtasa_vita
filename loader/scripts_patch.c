/* scripts_patch.c -- make hard modifications to game scripts
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <vitasdk.h>

#include <stdlib.h>
#include <string.h>

#include "dialog.h"
#include "config.h"

static char orig_opcode_00E1[6] = { 0xE1, 0x00, 0x04, 0x00, 0x04, 0x10 };
static char orig_opcode_80E1[6] = { 0xE1, 0x80, 0x04, 0x00, 0x04, 0x10 };

static char patch_0A90_nop[6] = { 0x90, 0x0A, 0x04, 0x40, 0x00, 0x00 };
static char patch_8A90_nop[6] = { 0x90, 0x8A, 0x04, 0x40, 0x00, 0x00 };

void patch_script(SceUID fd, char *orig, char *patch, size_t size, SceOff offset) {
  char *buf = malloc(size);
  sceIoPread(fd, buf, size, offset);
  if (memcmp(buf, orig, size) == 0)
    sceIoPwrite(fd, patch, size, offset);
  free(buf);
}

void patch_scripts(void) {
  // Fix ITB
  SceUID scm = sceIoOpen(SCRIPT_SCM_PATH, SCE_O_RDWR, 0);
  if (scm < 0)
    fatal_error("Error could not load script %s.", SCRIPT_SCM_PATH);
  patch_script(scm, orig_opcode_00E1, patch_0A90_nop, 6, 0x00066703);
  patch_script(scm, orig_opcode_00E1, patch_0A90_nop, 6, 0x0006675A);
  sceIoClose(scm);

  // Fix gym treadmill
  SceUID img = sceIoOpen(SCRIPT_IMG_PATH, SCE_O_RDWR, 0);
  if (img < 0)
    fatal_error("Error could not load script %s.", SCRIPT_IMG_PATH);
  patch_script(img, orig_opcode_00E1, patch_0A90_nop, 6, 0x0004C0FF);
  patch_script(img, orig_opcode_80E1, patch_8A90_nop, 6, 0x0004C131);
  sceIoClose(img);
}

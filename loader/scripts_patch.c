#include <psp2/io/fcntl.h>

#include <stdlib.h>

#include "dialog.h"
#include "config.h"

static uint8_t orig_opcode_00E1[6] = { 0xE1, 0x00, 0x04, 0x00, 0x04, 0x10 };
static uint8_t orig_opcode_80E1[6] = { 0xE1, 0x80, 0x04, 0x00, 0x04, 0x10 };

static uint8_t patch_0A90_nop[6] = { 0x90, 0x0A, 0x04, 0x40, 0x00, 0x00 };
static uint8_t patch_8A90_nop[6] = { 0x90, 0x8A, 0x04, 0x40, 0x00, 0x00 };

int check_original(SceUID fd, uint8_t *original_bytes, size_t size, SceOff offset) {
  uint8_t read_bytes[size];
  sceIoPread(fd, read_bytes, size, offset);
  int result = 1;
  for (int i = 0; i < size; i++) {
    if (read_bytes[i] != original_bytes[i]) {
      result = 0;
      break;
    }
  }
  return result;
}

void patch_scripts(void) {
  SceUID scm = sceIoOpen(SCRIPT_SCM_PATH, SCE_O_RDWR, 0);
  if (scm < 0)
    fatal_error("Error could not load game scripts: %s.", SCRIPT_SCM_PATH); // it will crash anyway

  // ITB
  if (check_original(scm, orig_opcode_00E1, sizeof(orig_opcode_00E1), 0x00066703))
    sceIoPwrite(scm, &patch_0A90_nop, sizeof(patch_0A90_nop), 0x00066703);
  if (check_original(scm, orig_opcode_00E1, sizeof(orig_opcode_00E1), 0x0006675A))
    sceIoPwrite(scm, &patch_0A90_nop, sizeof(patch_0A90_nop), 0x0006675A);
  sceIoClose(scm);

  SceUID img = sceIoOpen(SCRIPT_IMG_PATH, SCE_O_RDWR, 0);
  if (img < 0)
    fatal_error("Error could not load game scripts: %s.", SCRIPT_IMG_PATH);

  // Gym treadmill
  if (check_original(img, orig_opcode_00E1, sizeof(orig_opcode_00E1), 0x0004C0FF))
    sceIoPwrite(img, &patch_0A90_nop, sizeof(patch_0A90_nop), 0x0004C0FF);
  if (check_original(img, orig_opcode_80E1, sizeof(orig_opcode_80E1), 0x0004C131))
    sceIoPwrite(img, &patch_8A90_nop, sizeof(patch_8A90_nop), 0x0004C131);
  sceIoClose(img);
}
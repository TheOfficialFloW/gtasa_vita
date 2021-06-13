#include <psp2/io/fcntl.h>

#include <stdlib.h>

#include "dialog.h"
#include "config.h"
#include "sha1.h"

static uint32_t scm_orig_sha1[5] = { 0xFAB33E12, 0xB50F7E6B, 0x3BA52119, 0x948FEB82, 0x31A55BB6 };
static uint32_t img_orig_sha1[5] = { 0x8E314EED, 0x3E3EE789, 0xAC6A2A22, 0xCB1D8812, 0x80DADDDF };

static uint8_t hid_pressed_nop[6] = { 0x90, 0x0A, 0x04, 0x40, 0x00, 0x00 };
static uint8_t not_hid_pressed_nop[6] = { 0x90, 0x8A, 0x04, 0x40, 0x00, 0x00 };

int hash_equal(SceUID fd, uint32_t *original_hash) {
  uint32_t sha1[5];
  SHA1_CTX ctx;

  void *data;
  size_t size;

  int result = 1;

  size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);
  data = malloc(size);
  sceIoRead(fd, data, size);

  sha1_init(&ctx);
  sha1_update(&ctx, data, size);
  sha1_final(&ctx, (uint8_t *)sha1);

  for (int i = 0; i < 5; i++) {
    if (sha1[i] != original_hash[i]) {
      result = 0;
      break;
    }
  }
  free(data);
  
  return result;
}

void patch_scripts(void) {
  SceUID scm = sceIoOpen(SCRIPT_SCM_PATH, SCE_O_RDWR, 0);
  if (scm < 0)
    fatal_error("Error could not load game scripts: %s.", SCRIPT_SCM_PATH); // it will crash anyway

  if (hash_equal(scm, scm_orig_sha1)) {
    sceIoPwrite(scm, &hid_pressed_nop, sizeof(hid_pressed_nop), 0x00066703); // ITB
    sceIoPwrite(scm, &hid_pressed_nop, sizeof(hid_pressed_nop), 0x0006675A);
  }
  sceIoClose(scm);

  SceUID img = sceIoOpen(SCRIPT_IMG_PATH, SCE_O_RDWR, 0);
  if (img < 0)
    fatal_error("Error could not load game scripts: %s.", SCRIPT_IMG_PATH);

  if (hash_equal(img, img_orig_sha1)) {
    sceIoPwrite(img, &hid_pressed_nop, sizeof(hid_pressed_nop), 0x0004C0FF); // Gym treadmill
    sceIoPwrite(img, &not_hid_pressed_nop, sizeof(not_hid_pressed_nop), 0x0004C131);
  }
  sceIoClose(img);
}
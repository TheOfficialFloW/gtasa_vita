#ifndef __SCRIPTS_PATCH_H__
#define __SCRIPTS_PATCH_H__

int check_original(SceUID fd, uint8_t *original_bytes, size_t size, SceOff offset);

void patch_scripts(void);

#endif
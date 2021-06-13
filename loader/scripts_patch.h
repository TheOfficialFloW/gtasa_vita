#ifndef __SCRIPTS_PATCH_H__
#define __SCRIPTS_PATCH_H__

int hash_equal(SceUID fd, uint32_t *original_hash);

void patch_scripts(void);

#endif
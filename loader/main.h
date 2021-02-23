#ifndef __MAIN_H__
#define __MAIN_H__

#include <psp2/touch.h>
#include "config.h"

int debugPrintf(char *text, ...);

int ret0();
int OS_SystemChip();

SceUID _vshKernelSearchModuleByName(const char *, int *);

extern int input_cheat;

extern SceTouchPanelInfo panelInfoFront, panelInfoBack;

void patchControl(uint8_t btn, uint8_t action);

#endif

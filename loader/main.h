#ifndef __MAIN_H__
#define __MAIN_H__

#include <psp2/touch.h>
#include "config.h"
#include "so_util.h"

extern so_module gtasa_mod;

int debugPrintf(char *text, ...);

int ret0();
int OS_SystemChip();

int sceKernelChangeThreadCpuAffinityMask(SceUID thid, int cpuAffinityMask);

extern int input_cheat;

extern SceTouchPanelInfo panelInfoFront, panelInfoBack;

#endif

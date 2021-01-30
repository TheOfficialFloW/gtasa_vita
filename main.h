#ifndef __MAIN_H__
#define __MAIN_H__

#include <psp2/touch.h>
#include "config.h"

int debugPrintf(char *text, ...);

int ret0();
int OS_SystemChip();

extern SceTouchPanelInfo panelInfoFront, panelInfoBack;

#endif

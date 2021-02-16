/* dialog.c -- common dialog for error messages and cheats input
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/message_dialog.h>
#include <vitaGL.h>

#include <stdio.h>
#include <stdarg.h>

#include "main.h"
#include "dialog.h"

void fatal_error(const char *fmt, ...) {
  va_list list;
  char string[512];

  va_start(list, fmt);
  vsnprintf(string, sizeof(string), fmt, list);
  va_end(list);

  SceMsgDialogUserMessageParam msg_param;
  memset(&msg_param, 0, sizeof(msg_param));
  msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
  msg_param.msg = (SceChar8 *)string;

  SceMsgDialogParam param;
  sceMsgDialogParamInit(&param);
  _sceCommonDialogSetMagicNumber(&param.commonParam);
  param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
  param.userMsgParam = &msg_param;

  vglInit(8 * 1024 * 1024);

  if (sceMsgDialogInit(&param) == 0) {
    while (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
      vglSwapBuffers(GL_TRUE);
    }
    sceMsgDialogTerm();
  }

  sceKernelExitProcess(0);
  while (1);
}

/* dialog.c -- common dialog for error messages and cheats input
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/ime_dialog.h>
#include <psp2/message_dialog.h>
#include <vitaGL.h>

#include <stdio.h>
#include <stdarg.h>

#include "main.h"
#include "dialog.h"

static uint16_t ime_title_utf16[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t ime_initial_text_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t ime_input_text_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
static uint8_t ime_input_text_utf8[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];

void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
  for (int i = 0; src[i]; i++) {
    if ((src[i] & 0xFF80) == 0) {
      *(dst++) = src[i] & 0xFF;
    } else if((src[i] & 0xF800) == 0) {
      *(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
      *(dst++) = (src[i] & 0x3F) | 0x80;
    } else if((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
      *(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
      *(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
      *(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
      *(dst++) = (src[i + 1] & 0x3F) | 0x80;
      i += 1;
    } else {
      *(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
      *(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
      *(dst++) = (src[i] & 0x3F) | 0x80;
    }
  }

  *dst = '\0';
}

void utf8_to_utf16(const uint8_t *src, uint16_t *dst) {
  for (int i = 0; src[i];) {
    if ((src[i] & 0xE0) == 0xE0) {
      *(dst++) = ((src[i] & 0x0F) << 12) | ((src[i + 1] & 0x3F) << 6) | (src[i + 2] & 0x3F);
      i += 3;
    } else if ((src[i] & 0xC0) == 0xC0) {
      *(dst++) = ((src[i] & 0x1F) << 6) | (src[i + 1] & 0x3F);
      i += 2;
    } else {
      *(dst++) = src[i];
      i += 1;
    }
  }

  *dst = '\0';
}

int init_ime_dialog(const char *title, const char *initial_text) {
  memset(ime_title_utf16, 0, sizeof(ime_title_utf16));
  memset(ime_initial_text_utf16, 0, sizeof(ime_initial_text_utf16));
  memset(ime_input_text_utf16, 0, sizeof(ime_input_text_utf16));
  memset(ime_input_text_utf8, 0, sizeof(ime_input_text_utf8));

  utf8_to_utf16((uint8_t *)title, ime_title_utf16);
  utf8_to_utf16((uint8_t *)initial_text, ime_initial_text_utf16);

  SceImeDialogParam param;
  sceImeDialogParamInit(&param);

  param.supportedLanguages = 0x0001FFFF;
  param.languagesForced = SCE_TRUE;
  param.type = SCE_IME_TYPE_BASIC_LATIN;
  param.title = ime_title_utf16;
  param.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
  param.initialText = ime_initial_text_utf16;
  param.inputTextBuffer = ime_input_text_utf16;

  return sceImeDialogInit(&param);
}

char *get_ime_dialog_result(void) {
  if (sceImeDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
    return NULL;

  SceImeDialogResult result;
  memset(&result, 0, sizeof(SceImeDialogResult));
  sceImeDialogGetResult(&result);
  if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
    utf16_to_utf8(ime_input_text_utf16, ime_input_text_utf8);
  sceImeDialogTerm();
  // For some reason analog stick stops working after ime
  sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);

  return (char *)ime_input_text_utf8;
}

int init_msg_dialog(const char *msg) {
  SceMsgDialogUserMessageParam msg_param;
  memset(&msg_param, 0, sizeof(msg_param));
  msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
  msg_param.msg = (SceChar8 *)msg;

  SceMsgDialogParam param;
  sceMsgDialogParamInit(&param);
  _sceCommonDialogSetMagicNumber(&param.commonParam);
  param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
  param.userMsgParam = &msg_param;

  return sceMsgDialogInit(&param);
}

int get_msg_dialog_result(void) {
  if (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
    return 0;
  sceMsgDialogTerm();
  return 1;
}

void fatal_error(const char *fmt, ...) {
  va_list list;
  char string[512];

  va_start(list, fmt);
  vsnprintf(string, sizeof(string), fmt, list);
  va_end(list);

  vglInit(0);

  init_msg_dialog(string);

  while (!get_msg_dialog_result())
    vglSwapBuffers(GL_TRUE);

  sceKernelExitProcess(0);
  while (1);
}

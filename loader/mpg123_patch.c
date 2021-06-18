/* mpg123_patch.c -- mpg123 redirection
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>

#include <mpg123.h>

#include "main.h"
#include "so_util.h"

int mpg123_param_hook(mpg123_handle *mh, enum mpg123_parms key, long val, double fval) {
  if (config.enable_fuzzy_seek)
    val |= MPG123_FUZZY | MPG123_SEEKBUFFER | MPG123_GAPLESS;
  return mpg123_param(mh, key, val, fval);
}

void patch_mpg123(void) {
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_add_string"), (uintptr_t)mpg123_add_string);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_add_substring"), (uintptr_t)mpg123_add_substring);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_clip"), (uintptr_t)mpg123_clip);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_close"), (uintptr_t)mpg123_close);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_copy_string"), (uintptr_t)mpg123_copy_string);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_current_decoder"), (uintptr_t)mpg123_current_decoder);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_decode"), (uintptr_t)mpg123_decode);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_decode_frame"), (uintptr_t)mpg123_decode_frame);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_decoder"), (uintptr_t)mpg123_decoder);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_decoders"), (uintptr_t)mpg123_decoders);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_delete"), (uintptr_t)mpg123_delete);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_delete_pars"), (uintptr_t)mpg123_delete_pars);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_enc_from_id3"), (uintptr_t)mpg123_enc_from_id3);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_encodings"), (uintptr_t)mpg123_encodings);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_encsize"), (uintptr_t)mpg123_encsize);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_eq"), (uintptr_t)mpg123_eq);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_errcode"), (uintptr_t)mpg123_errcode);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_exit"), (uintptr_t)mpg123_exit);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_feature"), (uintptr_t)mpg123_feature);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_feed"), (uintptr_t)mpg123_feed);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_feedseek"), (uintptr_t)mpg123_feedseek);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_fmt"), (uintptr_t)mpg123_fmt);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_fmt_all"), (uintptr_t)mpg123_fmt_all);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_fmt_none"), (uintptr_t)mpg123_fmt_none);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_fmt_support"), (uintptr_t)mpg123_fmt_support);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_format"), (uintptr_t)mpg123_format);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_format_all"), (uintptr_t)mpg123_format_all);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_format_none"), (uintptr_t)mpg123_format_none);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_format_support"), (uintptr_t)mpg123_format_support);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_framebyframe_decode"), (uintptr_t)mpg123_framebyframe_decode);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_framebyframe_next"), (uintptr_t)mpg123_framebyframe_next);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_free_string"), (uintptr_t)mpg123_free_string);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_geteq"), (uintptr_t)mpg123_geteq);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_getformat"), (uintptr_t)mpg123_getformat);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_getpar"), (uintptr_t)mpg123_getpar);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_getparam"), (uintptr_t)mpg123_getparam);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_getstate"), (uintptr_t)mpg123_getstate);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_getvolume"), (uintptr_t)mpg123_getvolume);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_grow_string"), (uintptr_t)mpg123_grow_string);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_icy"), (uintptr_t)mpg123_icy);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_icy2utf8"), (uintptr_t)mpg123_icy2utf8);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_id3"), (uintptr_t)mpg123_id3);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_index"), (uintptr_t)mpg123_index);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_info"), (uintptr_t)mpg123_info);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_init"), (uintptr_t)mpg123_init);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_init_string"), (uintptr_t)mpg123_init_string);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_length"), (uintptr_t)mpg123_length);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_meta_check"), (uintptr_t)mpg123_meta_check);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_new"), (uintptr_t)mpg123_new);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_new_pars"), (uintptr_t)mpg123_new_pars);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_open"), (uintptr_t)mpg123_open);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_open_fd"), (uintptr_t)mpg123_open_fd);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_open_feed"), (uintptr_t)mpg123_open_feed);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_open_handle"), (uintptr_t)mpg123_open_handle);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_outblock"), (uintptr_t)mpg123_outblock);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_par"), (uintptr_t)mpg123_par);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_param"), (uintptr_t)mpg123_param_hook);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_parnew"), (uintptr_t)mpg123_parnew);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_plain_strerror"), (uintptr_t)mpg123_plain_strerror);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_position"), (uintptr_t)mpg123_position);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_rates"), (uintptr_t)mpg123_rates);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_read"), (uintptr_t)mpg123_read);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_replace_buffer"), (uintptr_t)mpg123_replace_buffer);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_replace_reader"), (uintptr_t)mpg123_replace_reader);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_replace_reader_handle"), (uintptr_t)mpg123_replace_reader_handle);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_reset_eq"), (uintptr_t)mpg123_reset_eq);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_resize_string"), (uintptr_t)mpg123_resize_string);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_safe_buffer"), (uintptr_t)mpg123_safe_buffer);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_scan"), (uintptr_t)mpg123_scan);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_seek"), (uintptr_t)mpg123_seek);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_seek_frame"), (uintptr_t)mpg123_seek_frame);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_set_filesize"), (uintptr_t)mpg123_set_filesize);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_set_index"), (uintptr_t)mpg123_set_index);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_set_string"), (uintptr_t)mpg123_set_string);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_set_substring"), (uintptr_t)mpg123_set_substring);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_store_utf8"), (uintptr_t)mpg123_store_utf8);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_strerror"), (uintptr_t)mpg123_strerror);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_strlen"), (uintptr_t)mpg123_strlen);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_supported_decoders"), (uintptr_t)mpg123_supported_decoders);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_tell"), (uintptr_t)mpg123_tell);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_tell_stream"), (uintptr_t)mpg123_tell_stream);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_tellframe"), (uintptr_t)mpg123_tellframe);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_timeframe"), (uintptr_t)mpg123_timeframe);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_tpf"), (uintptr_t)mpg123_tpf);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_volume"), (uintptr_t)mpg123_volume);
  hook_thumb(so_find_addr(&gtasa_mod, "mpg123_volume_change"), (uintptr_t)mpg123_volume_change);
}

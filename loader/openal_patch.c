/* openal_patch.c -- openal redirection
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>
#include <AL/efx.h>

#include "main.h"
#include "so_util.h"

ALCcontext *alcCreateContextHook(ALCdevice *dev, const ALCint *unused) {
  // override 22050hz with 44100hz in case someone wants high quality sounds
  const ALCint attr[] = { ALC_FREQUENCY, 44100, 0 };
  return alcCreateContext(dev, attr);
}

void patch_openal(void) {
  hook_thumb(so_symbol(&gtasa_mod, "alAuxiliaryEffectSlotf"), (uintptr_t)alAuxiliaryEffectSlotf);
  hook_thumb(so_symbol(&gtasa_mod, "alAuxiliaryEffectSlotfv"), (uintptr_t)alAuxiliaryEffectSlotfv);
  hook_thumb(so_symbol(&gtasa_mod, "alAuxiliaryEffectSloti"), (uintptr_t)alAuxiliaryEffectSloti);
  hook_thumb(so_symbol(&gtasa_mod, "alAuxiliaryEffectSlotiv"), (uintptr_t)alAuxiliaryEffectSlotiv);
  hook_thumb(so_symbol(&gtasa_mod, "alBuffer3f"), (uintptr_t)alBuffer3f);
  hook_thumb(so_symbol(&gtasa_mod, "alBuffer3i"), (uintptr_t)alBuffer3i);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferData"), (uintptr_t)alBufferData);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferSamplesSOFT"), (uintptr_t)alBufferSamplesSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferSubDataSOFT"), (uintptr_t)alBufferSubDataSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferSubSamplesSOFT"), (uintptr_t)alBufferSubSamplesSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferf"), (uintptr_t)alBufferf);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferfv"), (uintptr_t)alBufferfv);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferi"), (uintptr_t)alBufferi);
  hook_thumb(so_symbol(&gtasa_mod, "alBufferiv"), (uintptr_t)alBufferiv);
  hook_thumb(so_symbol(&gtasa_mod, "alDeferUpdatesSOFT"), (uintptr_t)alDeferUpdatesSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alDeleteAuxiliaryEffectSlots"), (uintptr_t)alDeleteAuxiliaryEffectSlots);
  hook_thumb(so_symbol(&gtasa_mod, "alDeleteBuffers"), (uintptr_t)alDeleteBuffers);
  hook_thumb(so_symbol(&gtasa_mod, "alDeleteEffects"), (uintptr_t)alDeleteEffects);
  hook_thumb(so_symbol(&gtasa_mod, "alDeleteFilters"), (uintptr_t)alDeleteFilters);
  hook_thumb(so_symbol(&gtasa_mod, "alDeleteSources"), (uintptr_t)alDeleteSources);
  hook_thumb(so_symbol(&gtasa_mod, "alDisable"), (uintptr_t)alDisable);
  hook_thumb(so_symbol(&gtasa_mod, "alDistanceModel"), (uintptr_t)alDistanceModel);
  hook_thumb(so_symbol(&gtasa_mod, "alDopplerFactor"), (uintptr_t)alDopplerFactor);
  hook_thumb(so_symbol(&gtasa_mod, "alDopplerVelocity"), (uintptr_t)alDopplerVelocity);
  hook_thumb(so_symbol(&gtasa_mod, "alEffectf"), (uintptr_t)alEffectf);
  hook_thumb(so_symbol(&gtasa_mod, "alEffectfv"), (uintptr_t)alEffectfv);
  hook_thumb(so_symbol(&gtasa_mod, "alEffecti"), (uintptr_t)alEffecti);
  hook_thumb(so_symbol(&gtasa_mod, "alEffectiv"), (uintptr_t)alEffectiv);
  hook_thumb(so_symbol(&gtasa_mod, "alEnable"), (uintptr_t)alEnable);
  hook_thumb(so_symbol(&gtasa_mod, "alFilterf"), (uintptr_t)alFilterf);
  hook_thumb(so_symbol(&gtasa_mod, "alFilterfv"), (uintptr_t)alFilterfv);
  hook_thumb(so_symbol(&gtasa_mod, "alFilteri"), (uintptr_t)alFilteri);
  hook_thumb(so_symbol(&gtasa_mod, "alFilteriv"), (uintptr_t)alFilteriv);
  hook_thumb(so_symbol(&gtasa_mod, "alGenBuffers"), (uintptr_t)alGenBuffers);
  hook_thumb(so_symbol(&gtasa_mod, "alGenEffects"), (uintptr_t)alGenEffects);
  hook_thumb(so_symbol(&gtasa_mod, "alGenFilters"), (uintptr_t)alGenFilters);
  hook_thumb(so_symbol(&gtasa_mod, "alGenSources"), (uintptr_t)alGenSources);
  hook_thumb(so_symbol(&gtasa_mod, "alGetAuxiliaryEffectSlotf"), (uintptr_t)alGetAuxiliaryEffectSlotf);
  hook_thumb(so_symbol(&gtasa_mod, "alGetAuxiliaryEffectSlotfv"), (uintptr_t)alGetAuxiliaryEffectSlotfv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetAuxiliaryEffectSloti"), (uintptr_t)alGetAuxiliaryEffectSloti);
  hook_thumb(so_symbol(&gtasa_mod, "alGetAuxiliaryEffectSlotiv"), (uintptr_t)alGetAuxiliaryEffectSlotiv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBoolean"), (uintptr_t)alGetBoolean);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBooleanv"), (uintptr_t)alGetBooleanv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBuffer3f"), (uintptr_t)alGetBuffer3f);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBuffer3i"), (uintptr_t)alGetBuffer3i);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBufferSamplesSOFT"), (uintptr_t)alGetBufferSamplesSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBufferf"), (uintptr_t)alGetBufferf);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBufferfv"), (uintptr_t)alGetBufferfv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBufferi"), (uintptr_t)alGetBufferi);
  hook_thumb(so_symbol(&gtasa_mod, "alGetBufferiv"), (uintptr_t)alGetBufferiv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetDouble"), (uintptr_t)alGetDouble);
  hook_thumb(so_symbol(&gtasa_mod, "alGetDoublev"), (uintptr_t)alGetDoublev);
  hook_thumb(so_symbol(&gtasa_mod, "alGetEffectf"), (uintptr_t)alGetEffectf);
  hook_thumb(so_symbol(&gtasa_mod, "alGetEffectfv"), (uintptr_t)alGetEffectfv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetEffecti"), (uintptr_t)alGetEffecti);
  hook_thumb(so_symbol(&gtasa_mod, "alGetEffectiv"), (uintptr_t)alGetEffectiv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetEnumValue"), (uintptr_t)alGetEnumValue);
  hook_thumb(so_symbol(&gtasa_mod, "alGetError"), (uintptr_t)alGetError);
  hook_thumb(so_symbol(&gtasa_mod, "alGetFilterf"), (uintptr_t)alGetFilterf);
  hook_thumb(so_symbol(&gtasa_mod, "alGetFilterfv"), (uintptr_t)alGetFilterfv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetFilteri"), (uintptr_t)alGetFilteri);
  hook_thumb(so_symbol(&gtasa_mod, "alGetFilteriv"), (uintptr_t)alGetFilteriv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetFloat"), (uintptr_t)alGetFloat);
  hook_thumb(so_symbol(&gtasa_mod, "alGetFloatv"), (uintptr_t)alGetFloatv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetInteger"), (uintptr_t)alGetInteger);
  hook_thumb(so_symbol(&gtasa_mod, "alGetIntegerv"), (uintptr_t)alGetIntegerv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetListener3f"), (uintptr_t)alGetListener3f);
  hook_thumb(so_symbol(&gtasa_mod, "alGetListener3i"), (uintptr_t)alGetListener3i);
  hook_thumb(so_symbol(&gtasa_mod, "alGetListenerf"), (uintptr_t)alGetListenerf);
  hook_thumb(so_symbol(&gtasa_mod, "alGetListenerfv"), (uintptr_t)alGetListenerfv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetListeneri"), (uintptr_t)alGetListeneri);
  hook_thumb(so_symbol(&gtasa_mod, "alGetListeneriv"), (uintptr_t)alGetListeneriv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetProcAddress"), (uintptr_t)alGetProcAddress);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSource3dSOFT"), (uintptr_t)alGetSource3dSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSource3f"), (uintptr_t)alGetSource3f);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSource3i"), (uintptr_t)alGetSource3i);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSource3i64SOFT"), (uintptr_t)alGetSource3i64SOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourcedSOFT"), (uintptr_t)alGetSourcedSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourcedvSOFT"), (uintptr_t)alGetSourcedvSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourcef"), (uintptr_t)alGetSourcef);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourcefv"), (uintptr_t)alGetSourcefv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourcei"), (uintptr_t)alGetSourcei);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourcei64SOFT"), (uintptr_t)alGetSourcei64SOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourcei64vSOFT"), (uintptr_t)alGetSourcei64vSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alGetSourceiv"), (uintptr_t)alGetSourceiv);
  hook_thumb(so_symbol(&gtasa_mod, "alGetString"), (uintptr_t)alGetString);
  hook_thumb(so_symbol(&gtasa_mod, "alIsAuxiliaryEffectSlot"), (uintptr_t)alIsAuxiliaryEffectSlot);
  hook_thumb(so_symbol(&gtasa_mod, "alIsBuffer"), (uintptr_t)alIsBuffer);
  hook_thumb(so_symbol(&gtasa_mod, "alIsBufferFormatSupportedSOFT"), (uintptr_t)alIsBufferFormatSupportedSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alIsEffect"), (uintptr_t)alIsEffect);
  hook_thumb(so_symbol(&gtasa_mod, "alIsEnabled"), (uintptr_t)alIsEnabled);
  hook_thumb(so_symbol(&gtasa_mod, "alIsExtensionPresent"), (uintptr_t)alIsExtensionPresent);
  hook_thumb(so_symbol(&gtasa_mod, "alIsFilter"), (uintptr_t)alIsFilter);
  hook_thumb(so_symbol(&gtasa_mod, "alIsSource"), (uintptr_t)alIsSource);
  hook_thumb(so_symbol(&gtasa_mod, "alListener3f"), (uintptr_t)alListener3f);
  hook_thumb(so_symbol(&gtasa_mod, "alListener3i"), (uintptr_t)alListener3i);
  hook_thumb(so_symbol(&gtasa_mod, "alListenerf"), (uintptr_t)alListenerf);
  hook_thumb(so_symbol(&gtasa_mod, "alListenerfv"), (uintptr_t)alListenerfv);
  hook_thumb(so_symbol(&gtasa_mod, "alListeneri"), (uintptr_t)alListeneri);
  hook_thumb(so_symbol(&gtasa_mod, "alListeneriv"), (uintptr_t)alListeneriv);
  hook_thumb(so_symbol(&gtasa_mod, "alProcessUpdatesSOFT"), (uintptr_t)alProcessUpdatesSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alSetConfigMOB"), (uintptr_t)ret0);
  hook_thumb(so_symbol(&gtasa_mod, "alSource3dSOFT"), (uintptr_t)alSource3dSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alSource3f"), (uintptr_t)alSource3f);
  hook_thumb(so_symbol(&gtasa_mod, "alSource3i"), (uintptr_t)alSource3i);
  hook_thumb(so_symbol(&gtasa_mod, "alSource3i64SOFT"), (uintptr_t)alSource3i64SOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcePause"), (uintptr_t)alSourcePause);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcePausev"), (uintptr_t)alSourcePausev);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcePlay"), (uintptr_t)alSourcePlay);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcePlayv"), (uintptr_t)alSourcePlayv);
  hook_thumb(so_symbol(&gtasa_mod, "alSourceQueueBuffers"), (uintptr_t)alSourceQueueBuffers);
  hook_thumb(so_symbol(&gtasa_mod, "alSourceRewind"), (uintptr_t)alSourceRewind);
  hook_thumb(so_symbol(&gtasa_mod, "alSourceRewindv"), (uintptr_t)alSourceRewindv);
  hook_thumb(so_symbol(&gtasa_mod, "alSourceStop"), (uintptr_t)alSourceStop);
  hook_thumb(so_symbol(&gtasa_mod, "alSourceStopv"), (uintptr_t)alSourceStopv);
  hook_thumb(so_symbol(&gtasa_mod, "alSourceUnqueueBuffers"), (uintptr_t)alSourceUnqueueBuffers);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcedSOFT"), (uintptr_t)alSourcedSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcedvSOFT"), (uintptr_t)alSourcedvSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcef"), (uintptr_t)alSourcef);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcefv"), (uintptr_t)alSourcefv);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcei"), (uintptr_t)alSourcei);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcei64SOFT"), (uintptr_t)alSourcei64SOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alSourcei64vSOFT"), (uintptr_t)alSourcei64vSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alSourceiv"), (uintptr_t)alSourceiv);
  hook_thumb(so_symbol(&gtasa_mod, "alSpeedOfSound"), (uintptr_t)alSpeedOfSound);
  hook_thumb(so_symbol(&gtasa_mod, "alcCaptureCloseDevice"), (uintptr_t)alcCaptureCloseDevice);
  hook_thumb(so_symbol(&gtasa_mod, "alcCaptureOpenDevice"), (uintptr_t)alcCaptureOpenDevice);
  hook_thumb(so_symbol(&gtasa_mod, "alcCaptureSamples"), (uintptr_t)alcCaptureSamples);
  hook_thumb(so_symbol(&gtasa_mod, "alcCaptureStart"), (uintptr_t)alcCaptureStart);
  hook_thumb(so_symbol(&gtasa_mod, "alcCaptureStop"), (uintptr_t)alcCaptureStop);
  hook_thumb(so_symbol(&gtasa_mod, "alcCloseDevice"), (uintptr_t)alcCloseDevice);
  hook_thumb(so_symbol(&gtasa_mod, "alcCreateContext"), (uintptr_t)alcCreateContextHook);
  hook_thumb(so_symbol(&gtasa_mod, "alcDestroyContext"), (uintptr_t)alcDestroyContext);
  hook_thumb(so_symbol(&gtasa_mod, "alcDeviceEnableHrtfMOB"), (uintptr_t)ret0);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetContextsDevice"), (uintptr_t)alcGetContextsDevice);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetCurrentContext"), (uintptr_t)alcGetCurrentContext);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetEnumValue"), (uintptr_t)alcGetEnumValue);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetError"), (uintptr_t)alcGetError);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetIntegerv"), (uintptr_t)alcGetIntegerv);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetProcAddress"), (uintptr_t)alcGetProcAddress);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetString"), (uintptr_t)alcGetString);
  hook_thumb(so_symbol(&gtasa_mod, "alcGetThreadContext"), (uintptr_t)alcGetThreadContext);
  hook_thumb(so_symbol(&gtasa_mod, "alcIsExtensionPresent"), (uintptr_t)alcIsExtensionPresent);
  hook_thumb(so_symbol(&gtasa_mod, "alcIsRenderFormatSupportedSOFT"), (uintptr_t)alcIsRenderFormatSupportedSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alcLoopbackOpenDeviceSOFT"), (uintptr_t)alcLoopbackOpenDeviceSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alcMakeContextCurrent"), (uintptr_t)alcMakeContextCurrent);
  hook_thumb(so_symbol(&gtasa_mod, "alcOpenDevice"), (uintptr_t)alcOpenDevice);
  hook_thumb(so_symbol(&gtasa_mod, "alcProcessContext"), (uintptr_t)alcProcessContext);
  hook_thumb(so_symbol(&gtasa_mod, "alcRenderSamplesSOFT"), (uintptr_t)alcRenderSamplesSOFT);
  hook_thumb(so_symbol(&gtasa_mod, "alcSetThreadContext"), (uintptr_t)alcSetThreadContext);
  hook_thumb(so_symbol(&gtasa_mod, "alcSuspendContext"), (uintptr_t)alcSuspendContext);
}

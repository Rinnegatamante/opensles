/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file SDL.c SDL platform implementation */

#include "sles_allinclusive.h"
#include <vitasdk.h>
#include <AL/al.h>
#include <AL/alc.h>

/** \brief Called by OpenAL to fill the next audio output buffer */
IEngine *slEngine;

static ALCdevice *ALDevice;
static ALvoid *ALContext;

uint8_t audio_buffers[SndFile_NUMBUFS][SndFile_BUFSIZE];

static int audioThread(unsigned int args, void* arg) {
	for (;;) {
		interface_lock_shared(slEngine);
		COutputMix *outputMix = slEngine->mOutputMix;
		interface_unlock_shared(slEngine);
		if (NULL != outputMix) {
			SLOutputMixExtItf OutputMixExt = &outputMix->mOutputMixExt.mItf;
			IOutputMixExt_FillBuffer(OutputMixExt, NULL, (SLuint32)SndFile_BUFSIZE*2);
		}
	}
	
	return sceKernelExitDeleteThread(0);
}

/** \brief Called during slCreateEngine */

void SDL_open(IEngine *thisEngine)
{
	slEngine = thisEngine;
		
	ALCint attrlist[6];
	attrlist[0] = ALC_FREQUENCY;
	attrlist[1] = 44100;
	attrlist[2] = ALC_SYNC;
	attrlist[3] = AL_FALSE;
	attrlist[4] = 0;
	
	ALDevice = alcOpenDevice(NULL);
	ALContext = alcCreateContext(ALDevice, attrlist);
	alcMakeContextCurrent(ALContext);

	ALfloat pos[] = { 0.0, 0.0, 0.0 };
	ALfloat vel[] = { 0.0, 0.0, 0.0 };
	ALfloat or[]  = { 0.0, 0.0, 1.0, 0.0, -1.0, 0.0 };
	alListenerf(AL_GAIN, 1.0);
	alListenerfv(AL_POSITION, pos);
	alListenerfv(AL_VELOCITY, vel);
	alListenerfv(AL_ORIENTATION, or);
  
	SceUID thd = sceKernelCreateThread("OpenSLES Playback", &audioThread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(thd, 0, NULL);
}


/** \brief Called during Object::Destroy */

void SDL_close(void)
{
}

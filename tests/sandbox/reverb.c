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

// Demonstrate environmental reverb and preset reverb on an output mix and audio player

#include "SLES/OpenSLES.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Table of I3DL2 named environmental reverb settings

typedef struct {
    const char *mName;
    SLEnvironmentalReverbSettings mSettings;
} Pair;

#define _(name) {#name, SL_I3DL2_ENVIRONMENT_PRESET_##name},

Pair pairs[] = {
    _(DEFAULT)
    _(GENERIC)
    _(PADDEDCELL)
    _(ROOM)
    _(BATHROOM)
    _(LIVINGROOM)
    _(STONEROOM)
    _(AUDITORIUM)
    _(CONCERTHALL)
    _(CAVE)
    _(ARENA)
    _(HANGAR)
    _(CARPETEDHALLWAY)
    _(HALLWAY)
    _(STONECORRIDOR)
    _(ALLEY)
    _(FOREST)
    _(CITY)
    _(MOUNTAINS)
    _(QUARRY)
    _(PLAIN)
    _(PARKINGLOT)
    _(SEWERPIPE)
    _(UNDERWATER)
    _(SMALLROOM)
    _(MEDIUMROOM)
    _(LARGEROOM)
    _(MEDIUMHALL)
    _(LARGEHALL)
    _(PLATE)
};

// Reverb parameters for output mix
SLuint16 mixPresetNumber = ~0;
char *mixEnvName = NULL;
SLEnvironmentalReverbSettings mixEnvSettings;

// Reverb parameters for audio player
SLuint16 playerPreset = ~0;
char *playerName = NULL;
SLEnvironmentalReverbSettings playerSettings;

// Compare two environmental reverb settings structures.
// Returns true if the settings are identical, or false if they are different.

#define bool int
bool slesutCompareEnvronmentalReverbSettings(
        const SLEnvironmentalReverbSettings *settings1,
        const SLEnvironmentalReverbSettings *settings2)
{
    return
        (settings1->roomLevel == settings2->roomLevel) &&
        (settings1->roomHFLevel == settings2->roomHFLevel) &&
        (settings1->decayTime == settings2->decayTime) &&
        (settings1->decayHFRatio == settings2->decayHFRatio) &&
        (settings1->reflectionsLevel == settings2->reflectionsLevel) &&
        (settings1->reflectionsDelay == settings2->reflectionsDelay) &&
        (settings1->reverbLevel == settings2->reverbLevel) &&
        (settings1->reverbDelay == settings2->reverbDelay) &&
        (settings1->diffusion == settings2->diffusion) &&
        (settings1->density == settings2->density);
}

// Print an environmental reverb settings structure.

void slesutPrintEnvironmentalReverbSettings( const SLEnvironmentalReverbSettings *settings)
{
    printf("roomLevel: %u\n", settings->roomLevel);
    printf("roomHFLevel: %u\n", settings->roomHFLevel);
    printf("decayTime: %lu\n", settings->decayTime);
    printf("decayHFRatio: %u\n", settings->decayHFRatio);
    printf("reflectionsLevel: %u\n", settings->reflectionsLevel);
    printf("reflectionsDelay: %lu\n", settings->reflectionsDelay);
    printf("reverbLevel: %u\n", settings->reverbLevel);
    printf("reverbDelay: %lu\n", settings->reverbDelay);
    printf("diffusion: %u\n", settings->diffusion);
    printf("density: %u\n", settings->density);
}

// Main program

int main(int argc, char **argv)
{
    SLresult result;

    // process command line parameters
    char *prog = argv[0];
    int i;
    for (i = 1; i < argc; ++i) {
        char *arg = argv[i];
        if (arg[0] != '-')
            break;
        if (!strncmp(arg, "--mix-preset=", 13)) {
            mixPresetNumber = atoi(&arg[13]);
        } else if (!strncmp(arg, "--mix-name=", 11)) {
            mixEnvName = &arg[11];
        } else {
            fprintf(stderr, "%s: unknown option %s ignored\n", prog, arg);
        }
    }
    if (argc - i != 1) {
        fprintf(stderr, "usage: %s --mix-preset=# --mix-name=name filename\n", prog);
        exit(EXIT_FAILURE);
    }
    char *pathname = argv[i];

    if (NULL != mixEnvName) {
        unsigned j;
        for (j = 0; j < sizeof(pairs) / sizeof(pairs[0]); ++j) {
            //printf("comparing %s %s\n", mixEnvName, pairs[j].mName);
            if (!strcasecmp(mixEnvName, pairs[j].mName)) {
                mixEnvSettings = pairs[j].mSettings;
                goto found;
            }
        }
        fprintf(stderr, "%s: reverb name %s not found\n", prog, mixEnvName);
        exit(EXIT_FAILURE);
found:
        printf("Using reverb name %s\n", mixEnvName);
    }

    // create engine
    SLObjectItf engineObject;
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    SLEngineItf engineEngine;
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);

    // create output mix
    SLInterfaceID mix_ids[2];
    SLboolean mix_req[2];
    SLuint32 count = 0;
    if (mixPresetNumber != ((SLuint16) ~0)) {
        mix_req[count] = SL_BOOLEAN_TRUE;
        mix_ids[count++] = SL_IID_PRESETREVERB;
    }
    if (mixEnvName != NULL) {
        mix_req[count] = SL_BOOLEAN_TRUE;
        mix_ids[count++] = SL_IID_ENVIRONMENTALREVERB;
    }
    SLObjectItf mixObject;
    result = (*engineEngine)->CreateOutputMix(engineEngine, &mixObject, count, mix_ids, mix_req);
    assert(SL_RESULT_SUCCESS == result);
    result = (*mixObject)->Realize(mixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // configure preset reverb on output mix
    SLPresetReverbItf mixPresetReverb;
    if (mixPresetNumber != ((SLuint16) ~0)) {
        result = (*mixObject)->GetInterface(mixObject, SL_IID_PRESETREVERB, &mixPresetReverb);
        assert(SL_RESULT_SUCCESS == result);
        SLuint16 getPresetReverb = 12345;
        result = (*mixPresetReverb)->GetPreset(mixPresetReverb, &getPresetReverb);
        assert(SL_RESULT_SUCCESS == result);
        printf("Output mix default preset reverb %u\n", getPresetReverb);
        result = (*mixPresetReverb)->SetPreset(mixPresetReverb, mixPresetNumber);
        if (SL_RESULT_SUCCESS == result) {
            result = (*mixPresetReverb)->GetPreset(mixPresetReverb, &getPresetReverb);
            assert(SL_RESULT_SUCCESS == result);
            assert(getPresetReverb == mixPresetNumber);
            printf("output mix preset reverb successfully changed to %u\n", mixPresetNumber);
        } else
            printf("Unable to set preset reverb to %u, result=%lu\n", mixPresetNumber, result);
    }

    // configure environmental reverb on output mix
    SLEnvironmentalReverbItf mixEnvironmentalReverb;
    if (mixEnvName != NULL) {
        result = (*mixObject)->GetInterface(mixObject, SL_IID_ENVIRONMENTALREVERB,
                &mixEnvironmentalReverb);
        assert(SL_RESULT_SUCCESS == result);
        SLEnvironmentalReverbSettings getSettings;
        result = (*mixEnvironmentalReverb)->GetEnvironmentalReverbProperties(mixEnvironmentalReverb,
                &getSettings);
        assert(SL_RESULT_SUCCESS == result);
        printf("Output mix default environmental reverb settings\n");
        printf("------------------------------------------------\n");
        slesutPrintEnvironmentalReverbSettings(&getSettings);
        printf("\n");
        result = (*mixEnvironmentalReverb)->SetEnvironmentalReverbProperties(mixEnvironmentalReverb,
                &mixEnvSettings);
        assert(SL_RESULT_SUCCESS == result);
        printf("Output mix new environmental reverb settings\n");
        printf("--------------------------------------------\n");
        slesutPrintEnvironmentalReverbSettings(&mixEnvSettings);
        printf("\n");
        result = (*mixEnvironmentalReverb)->GetEnvironmentalReverbProperties(mixEnvironmentalReverb,
                &getSettings);
        assert(SL_RESULT_SUCCESS == result);
        printf("Output mix read environmental reverb settings\n");
        printf("--------------------------------------------\n");
        slesutPrintEnvironmentalReverbSettings(&getSettings);
        printf("\n");
        // assert(slesutCompareEnvronmentalReverbSettings(&getSettings, &mixEnvSettings));
    }

    // create audio player
    SLDataLocator_URI locURI = {SL_DATALOCATOR_URI, pathname};
    SLDataFormat_MIME dfMIME = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
    SLDataSource audioSrc = {&locURI, &dfMIME};
    SLDataLocator_OutputMix locOutputMix = {SL_DATALOCATOR_OUTPUTMIX, mixObject};
    SLDataSink audioSnk = {&locOutputMix, NULL};
    SLInterfaceID player_ids[3];
    SLboolean player_req[3];
    count = 0;
    if (playerPreset != ((SLuint16) ~0)) {
        player_req[count] = SL_BOOLEAN_TRUE;
        player_ids[count++] = SL_IID_PRESETREVERB;
    }
    if (playerName != NULL) {
        player_req[count] = SL_BOOLEAN_TRUE;
        player_ids[count++] = SL_IID_ENVIRONMENTALREVERB;
    }
    if (mixPresetNumber != ((SLuint16) ~0) || mixEnvName != NULL) {
        player_req[count] = SL_BOOLEAN_TRUE;
        player_ids[count++] = SL_IID_EFFECTSEND;
    }
    SLObjectItf playerObject;
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audioSrc,
        &audioSnk, count, player_ids, player_req);
    assert(SL_RESULT_SUCCESS == result);
    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
    SLPlayItf playerPlay;
    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
    assert(SL_RESULT_SUCCESS == result);

    // get duration
    SLmillisecond duration;
    result = (*playerPlay)->GetDuration(playerPlay, &duration);
    assert(SL_RESULT_SUCCESS == result);
    if (SL_TIME_UNKNOWN == duration)
        printf("first attempt at duration: unknown\n");
    else
        printf("first attempt at duration: %.1f seconds\n", duration / 1000.0);

    // set play state to paused to enable pre-fetch so we can get a more reliable duration
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PAUSED);
    assert(SL_RESULT_SUCCESS == result);
    usleep(1000000);
    result = (*playerPlay)->GetDuration(playerPlay, &duration);
    assert(SL_RESULT_SUCCESS == result);
    if (SL_TIME_UNKNOWN == duration)
        printf("second attempt at duration: unknown\n");
    else
        printf("second attempt at duration: %.1f seconds\n", duration / 1000.0);

    // if reverb is on output mix (aux effect), then enable it
    if (mixPresetNumber != ((SLuint16) ~0) || mixEnvName != NULL) {
        SLEffectSendItf playerEffectSend;
        result = (*playerObject)->GetInterface(playerObject, SL_IID_EFFECTSEND, &playerEffectSend);
        assert(SL_RESULT_SUCCESS == result);
        if (mixPresetNumber != ((SLuint16) ~0)) {
            result = (*playerEffectSend)->EnableEffectSend(playerEffectSend, mixPresetReverb,
                    SL_BOOLEAN_TRUE, (SLmillibel) 0);
            assert(SL_RESULT_SUCCESS == result);
        }
        if (mixEnvName != NULL) {
            result = (*playerEffectSend)->EnableEffectSend(playerEffectSend, mixEnvironmentalReverb,
                    SL_BOOLEAN_TRUE, (SLmillibel) 0);
            assert(SL_RESULT_SUCCESS == result);
        }
    }

    // start audio playing
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);

    // wait for audio to finish playing
    SLuint32 state;
    for (;;) {
        result = (*playerPlay)->GetPlayState(playerPlay, &state);
        assert(SL_RESULT_SUCCESS == result);
        if (SL_PLAYSTATE_PLAYING != state)
            break;
        usleep(5000000);
     }
    assert(SL_PLAYSTATE_PAUSED == state);

    return EXIT_SUCCESS;
}

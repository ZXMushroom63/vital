/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/
#include <emscripten.h>
#include <emscripten/bind.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

namespace juce
{

extern "C"
{

int bSizeGlobal = 128;

EM_JS(int, getEmsdkSamplerate, (), {
  return globalThis.VIAL_TARGET_SAMPLERATE || 44100;
});

EM_JS(int, getEmsdkChannelCount, (), {
  return Math.min(2, Math.max(1, Math.floor(globalThis.VIAL_CHANNEL_COUNT))) || 1;
});

static void silentErrorHandler (const char*, int, const char*, int, const char*,...) {}
#define JUCE_EMSDK_LOG(dbgtext)   { juce::String tempDbgBuf ("EMSDK_Audio: "); tempDbgBuf << dbgtext; Logger::writeToLog (tempDbgBuf); DBG (tempDbgBuf); }

//==============================================================================
class EMSDKAudioIODevice   : public AudioIODevice
{
public:
    EMSDKAudioIODevice (const String& deviceName,
                       const String& deviceTypeName,
                       const String& inputDeviceID,
                       const String& outputDeviceID)
        : AudioIODevice (deviceName, deviceTypeName),
          inputId (inputDeviceID),
          outputId (outputDeviceID)
    {
    }

    ~EMSDKAudioIODevice() override
    {
        close();
    }

    StringArray getOutputChannelNames() override            {
        int channelCount = getEmsdkChannelCount();
        StringArray r;
        if (channelCount == 1) {
            r.add("Out");
        } else {
            r.add("LeftOut");
            r.add("RightOut");
        }
        return r;
    }
    StringArray getInputChannelNames() override             { 
        StringArray r;
        return r;
    }

    Array<double> getAvailableSampleRates() override        {
        Array<double> r;
        r.add((double) getEmsdkSamplerate());
        return r;        
    }

    Array<int> getAvailableBufferSizes() override
    {
        Array<int> r;
        r.add(bSizeGlobal);
        return r;
    }

    int getDefaultBufferSize() override                      { return bSizeGlobal; }

    String open (const BigInteger& inputChannels,
                 const BigInteger& outputChannels,
                 double sampleRate,
                 int bufferSizeSamples) override
    {
        close();

        if (bufferSizeSamples <= 0)
            bufferSizeSamples = getDefaultBufferSize();

        if (sampleRate <= 0)
        {
            sampleRate = (double) getEmsdkSamplerate();
        }

        JUCE_EMSDK_LOG("Todo: something about opening audio context somehow?? idk")

        //internal.open (inputChannels, outputChannels,
        //               sampleRate, bufferSizeSamples);

        isOpen_ = true;
        return "";
    }

    void close() override
    {
        JUCE_EMSDK_LOG("Pretending to close web audio context.")
        stop();
        //internal.close();
        isOpen_ = false;
    }

    bool isOpen() override                           { return isOpen_; }
    bool isPlaying() override                        { return isStarted; }
    String getLastError() override                   { return ""; }

    int getCurrentBufferSizeSamples() override       { return bSizeGlobal; }
    double getCurrentSampleRate() override           { return getEmsdkSamplerate(); }
    int getCurrentBitDepth() override                { return 32; }

    BigInteger getActiveOutputChannels() const override    { return getEmsdkChannelCount(); }
    BigInteger getActiveInputChannels() const override     { return 0; }

    int getOutputLatencyInSamples() override         { return bSizeGlobal; }
    int getInputLatencyInSamples() override          { return 0; }

    int getXRunCount() const noexcept override       { return 0; } //X RUN (definition for me): either an underrun (system runs out of data to play) or an overrun (system cannot keep up with the data being queued)

    void start (AudioIODeviceCallback* callback) override
    {
        if (! isOpen_) {
            callback = nullptr;
        }

        if (callback != nullptr) {
            JUCE_EMSDK_LOG("Received callback, preparing to send!");
            callback->audioDeviceAboutToStart (this);
            internalCallback = callback;
        }

        isStarted = callback != nullptr;
    }

    void stop() override
    {
        auto oldCallback = internalCallback;

        start (nullptr);

        if (oldCallback != nullptr)
            oldCallback->audioDeviceStopped();
    }

    String inputId, outputId;
    static AudioIODeviceCallback* internalCallback;
private:
    bool isOpen_ = false, isStarted = false;
};
AudioIODeviceCallback* EMSDKAudioIODevice::internalCallback = nullptr;

//==============================================================================
class EMSDKAudioIODeviceType  : public AudioIODeviceType
{
public:
    EMSDKAudioIODeviceType (bool onlySoundcards, const String& deviceTypeName)
        : AudioIODeviceType (deviceTypeName),
          listOnlySoundcards (onlySoundcards)
    {
        JUCE_EMSDK_LOG("EMSDK Audio Device Type registered.");
    }

    ~EMSDKAudioIODeviceType()
    {
    }

    //==============================================================================
    void scanForDevices()
    {
        if (hasScanned)
            return;

        hasScanned = true;
        inputNames.clear();
        inputIds.clear();
        outputNames.clear();
        outputIds.clear();

        JUCE_EMSDK_LOG ("EMSDK scanForDevices() !!!");

        if (!listOnlySoundcards) {
            getCurrentEmsdkCtx();
        }

        inputNames.appendNumbersToDuplicates (false, true);
        outputNames.appendNumbersToDuplicates (false, true);
    }

    StringArray getDeviceNames (bool wantInputNames) const
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        return wantInputNames ? inputNames : outputNames;
    }

    int getDefaultDeviceIndex (bool forInput) const
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        auto idx = (forInput ? inputIds : outputIds).indexOf ("default");
        return idx >= 0 ? idx : 0;
    }

    bool hasSeparateInputsAndOutputs() const    { return true; }

    int getIndexOfDevice (AudioIODevice* device, bool asInput) const
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        if (auto* d = dynamic_cast<EMSDKAudioIODevice*> (device))
            return asInput ? inputIds.indexOf (d->inputId)
                           : outputIds.indexOf (d->outputId);

        return -1;
    }

    AudioIODevice* createDevice (const String& outputDeviceName,
                                 const String& inputDeviceName)
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        auto inputIndex = inputNames.indexOf (inputDeviceName);
        auto outputIndex = outputNames.indexOf (outputDeviceName);

        String deviceName (outputIndex >= 0 ? outputDeviceName
                                            : inputDeviceName);

        if (inputIndex >= 0 || outputIndex >= 0)
            return new EMSDKAudioIODevice (deviceName, getTypeName(),
                                          inputIds [inputIndex],
                                          outputIds [outputIndex]);

        return nullptr;
    }

private:
    //==============================================================================
    StringArray inputNames, outputNames, inputIds, outputIds;
    bool hasScanned = false;
    const bool listOnlySoundcards;

    bool testDevice (const String& id, const String& outputName, const String& inputName)
    {
        return true;
    }

    /* Get the EMSDK context
    */
    void getCurrentEmsdkCtx()
    {
        outputIds.add("default");
        outputNames.add("WebAudio");
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EMSDKAudioIODeviceType)
};

uint8_t audioLock = 0;
uint8_t consumption = 0;
float*** audioStack = nullptr;
float*** readableStack = nullptr;

EM_JS(void, sendAudioStack, (uint8_t* lock, uint8_t* consumption, float*** audioStack), {
    globalThis._V_AUDIO_LOCK_PTR = lock;
    globalThis._V_AUDIO_CONSUMPTION_PTR = consumption;
    globalThis._V_AUDIO_PTRSTACK = audioStack;
});

void acquire_lock(uint8_t* lockRef) {
    while (*lockRef != 0) {
        usleep(3000); //3ms
        // spinlock
    }
}

void release_lock(uint8_t* lockRef) {
    *lockRef = 0;
}

typedef struct {
    float*** audioStack;
    int channelCount;
    int stackSize;
    double delayMilliseconds; //1000us = 1ms = 0.001s
    uint8_t* audioLockRef;
    uint8_t* consumptionRef;
    float*** readableStack;
    int bufSize;
} audiothread_params;

void shiftReadableStack(float*** readableStack, int size, float** newEntry) {
    for (int i = 1; i < size; ++i) {
        readableStack[i - 1] = readableStack[i];
    }
    readableStack[size - 1] = newEntry;
}

int getLowestWriteIdx(float*** readableStack, int targetReadIndex) { //targetFillLevel - 1
    int index = targetReadIndex;
    while ((readableStack[index] == nullptr) && (index >= 0)) {
        index--;
    }
    return index + 1;
}

void* audioThread(void* arg) {
    audiothread_params* params = (audiothread_params*)arg;
    int c = params->channelCount;
    double delayMilliseconds = params->delayMilliseconds;
    float*** audioStack = params->audioStack;
    float*** readableStack = params->readableStack;
    uint8_t* lock = params->audioLockRef;
    uint8_t* consumption = params->consumptionRef;
    int bSize = params->bufSize;

    int fillLevel = 0;
    int targetFillLevel = params->stackSize;
    int targetFillIndex = targetFillLevel - 1;
    int stackSize = targetFillLevel * 2;

    while (1) {
        acquire_lock(lock);
        fillLevel = std::max(fillLevel - (int)(*consumption), 0);
        for (int con = 0; con < (*consumption); con++) {
            shiftReadableStack(readableStack, stackSize, nullptr);
            shiftReadableStack(audioStack, stackSize, audioStack[0]); //circular
        }
        *consumption = 0;
        double start_time = emscripten_get_now();
        while (readableStack[targetFillIndex] == nullptr) {
            int writeIndex = getLowestWriteIdx(readableStack, targetFillIndex);
            EMSDKAudioIODevice::internalCallback->audioDeviceIOCallback (
                nullptr,
                0,
                audioStack[writeIndex],
                c,
                bSize
            );
            readableStack[writeIndex] = audioStack[writeIndex];
        }
        release_lock(lock);

        double end_time = emscripten_get_now();
        
        double duration = end_time - start_time;
        usleep(1000*std::max(delayMilliseconds - duration, 1.0));
    }
    return NULL;
}

EMSCRIPTEN_KEEPALIVE
void setupAudioThread(int c, int stackSize, int bSize, double clockspeedMult) {
    bSizeGlobal = bSize;
    audioStack = (float***)malloc(sizeof(float**) * (stackSize * 2));
    readableStack = (float***)malloc(sizeof(float**) * (stackSize * 2));

    // that's a lot of nesting ;-;
    for (int s = 0; s < (stackSize * 2); s++) {
        audioStack[s] = (float**)malloc(sizeof(float*) * c);
        for (int i=0; i < c; i++) {
            audioStack[s][i] = (float*)malloc(sizeof(float) * bSize);
            for (int j = 0; j < bSize; j++) {
                audioStack[s][i][j] = 0.0f;
            }
        }
    }

    audiothread_params tparams;
    tparams.audioStack = audioStack;
    tparams.channelCount = c;
    tparams.stackSize = stackSize;
    tparams.delayMilliseconds = (((double)bSize)*1000.00/((double)getEmsdkSamplerate())) / clockspeedMult;
    tparams.audioLockRef = &audioLock;
    tparams.consumptionRef = &consumption;
    tparams.readableStack = readableStack;
    tparams.bufSize = bSize;

    pthread_t periodic_thread;
    pthread_create(&periodic_thread, NULL, audioThread, (void*)&tparams);

    sendAudioStack(&audioLock, &consumption, audioStack);
    
    return;
}
}


} // namespace juce

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
        r.add(512);
        return r;
    }

    int getDefaultBufferSize() override                      { return 512; }

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

    int getCurrentBufferSizeSamples() override       { return 512; }
    double getCurrentSampleRate() override           { return getEmsdkSamplerate(); }
    int getCurrentBitDepth() override                { return 32; }

    BigInteger getActiveOutputChannels() const override    { return getEmsdkChannelCount(); }
    BigInteger getActiveInputChannels() const override     { return 0; }

    int getOutputLatencyInSamples() override         { return 512; }
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

float** outputChannelDataForCallback = nullptr;

typedef struct {
    float** outputBuffers;
    int channelCount;
    double delayMilliseconds; //1000us = 1ms = 0.001s
} audiothread_params;

void* audioThread(void* arg) {
    audiothread_params* params = (audiothread_params*)arg;
    bool fallingBehind = false;
    int c = params->channelCount;
    double delayMilliseconds = params->delayMilliseconds;
    std::cout << "Targ delay(ms): " << delayMilliseconds << std::endl;
    float** outputBuffers = params->outputBuffers;
    int logIndex = 0;
    int loggingInterval = 128;
    std::cout << "Master Ptr: " << outputBuffers << std::endl;
    for (int i = 0; i < c; i++) {
        std::cout << "Channel " << i << ": " << outputBuffers[i] << std::endl;
    }

    double timerError = 0;
    double correctionIntensity = 0.33;

    while (1) {
        double start_time = emscripten_get_now();

        if (fallingBehind) {
            fallingBehind = false;
            std::cout << "Falling behind, skipping round..." << std::endl;
        } else {
            EMSDKAudioIODevice::internalCallback->audioDeviceIOCallback (
                nullptr,
                0,
                outputBuffers,
                c,
                512
            );
        }

        double end_time = emscripten_get_now();
        
        double duration = end_time - start_time;
        if (duration > delayMilliseconds) {
            fallingBehind = false;
        }
        usleep(1000*std::max(delayMilliseconds - duration - (timerError * 1.5), 0.5));


        double actualEndTime = emscripten_get_now();

        double errorValue = ((actualEndTime - start_time) - delayMilliseconds);
        timerError = (errorValue - timerError) * correctionIntensity + timerError;

        logIndex++;
        if (logIndex >= loggingInterval) {
            logIndex = 0;
            std::cout << "Interval: " << (actualEndTime - start_time) << "(target=" << delayMilliseconds << "). processing=" << duration << "; errorCorrection=" << timerError << std::endl;
        }
    }
    return NULL;
}

EMSCRIPTEN_KEEPALIVE
float** setupAudioThread(int c) {
    outputChannelDataForCallback = (float**)malloc(sizeof(float*) * c);
    std::cout << "Allocated memory for " << c << " audio channels." << std::endl;;
    for (int i=0; i < c; i++) {
        outputChannelDataForCallback[i] = (float*)malloc(sizeof(float) * 512);
        for (int j = 0; j < 512; j++) {
            outputChannelDataForCallback[i][j] = 0.0f;
        }
    }

    audiothread_params tparams;
    tparams.outputBuffers = outputChannelDataForCallback;
    tparams.channelCount = c;
    tparams.delayMilliseconds = (512.00*1000.00/((double)getEmsdkSamplerate()));

    pthread_t periodic_thread;
    pthread_create(&periodic_thread, NULL, audioThread, (void*)&tparams);
    
    return outputChannelDataForCallback;

    // MEANWHILE, IN PEACEFUL JAVASCRIPT LAND
    /*
        let bufferPtr = -1;
        let errorPtr = -1;
        globalThis.VIAL_AUDIOCALLBACK_DATA = (buffers, errors) => {
            bufferPtr = buffers;
            errorPtr = errors;
        };
        Vial._setupAudioThread(globalThis.VIAL_CHANNEL_COUNT); //get the most recent audio block output
        const channelPtrs = new Uint32Array(globalThis.VIAL_CHANNEL_COUNT);
        const channelBuffers = [];

        for (let c=0; c < globalThis.VIAL_CHANNEL_COUNT; c++) {
            channelPtrs[c] = Vial.HEAPU32[bufferPtr / 4 + c];
            channelBuffers.push(Vial.HEAPF32.subarray(channelPtrs[c] / 4, channelPtrs[c] / 4 + 512));
        }
    */
}
}


} // namespace juce

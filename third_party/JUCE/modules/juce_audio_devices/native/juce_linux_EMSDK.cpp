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
    String getLastError() override                   { return "nothing here"; }

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

        if (outBuffer == nullptr) {
            outBuffer = new float[512];
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
    static float* outBuffer;
private:
    bool isOpen_ = false, isStarted = false;
};
AudioIODeviceCallback* EMSDKAudioIODevice::internalCallback = nullptr;
float* EMSDKAudioIODevice::outBuffer = nullptr;

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

EMSCRIPTEN_KEEPALIVE
float* audioCallback() {
    float* outBuf = EMSDKAudioIODevice::outBuffer;
    if (outBuf == nullptr) {
        return nullptr;
    }
    Array<float*> outputChannelDataForCallback;
    EMSDKAudioIODevice::internalCallback->audioDeviceIOCallback (
        nullptr,
        0,
        outputChannelDataForCallback.getRawDataPointer(),
        512,
        512
    );
    
    std::fill(outBuf, outBuf + 512, 0.0f);
    if (outputChannelDataForCallback.size() == 512) {
        for (int i = 0; i < 512; ++i) { //assuming 512 in length, if it isn't, we have bigger problems
            if (outputChannelDataForCallback[i] != nullptr) {
                outBuf[i] = *(outputChannelDataForCallback[i]);
            }
        }
    }
    return outBuf;

    // MEANWHILE, IN PEACEFUL JAVASCRIPT LAND
    /*
        const ptr = Module._audioCallback(); //get the most recent audio block output
        if (ptr === -1) {
            // exit early, audio system not initialised yet
        }
        const pcm_copy = new Float32Array(Module.HEAP32.buffer, ptr, 512); // copy
        const pcm_ref = Module.HEAP32.subarray(ptr, ptr + 512); // reference (fast ver.)
    */
}

}

} // namespace juce

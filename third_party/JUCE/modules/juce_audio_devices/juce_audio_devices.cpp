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

#ifdef JUCE_AUDIO_DEVICES_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#define JUCE_CORE_INCLUDE_OBJC_HELPERS 1
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#define JUCE_CORE_INCLUDE_JNI_HELPERS 1
#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#define JUCE_EVENTS_INCLUDE_WIN32_MESSAGE_WINDOW 1

#ifndef JUCE_USE_WINRT_MIDI
 #define JUCE_USE_WINRT_MIDI 0
#endif

#if JUCE_USE_WINRT_MIDI
 #define JUCE_EVENTS_INCLUDE_WINRT_WRAPPER 1
#endif

#include "juce_audio_devices.h"

#include "native/juce_MidiDataConcatenator.h"


#if JUCE_LINUX
  //#include <alsa/asoundlib.h>
  #include "native/juce_linux_EMSDK.cpp"
  #include "native/juce_linux_Midi.cpp" //MARKER WOO LOOKIE HERE MIDI SUPPORT LATER
#endif

#if ! JUCE_SYSTEMAUDIOVOL_IMPLEMENTED
namespace juce
{
    // None of these methods are available. (On Windows you might need to enable WASAPI for this)
    float JUCE_CALLTYPE SystemAudioVolume::getGain()         { jassertfalse; return 0.0f; }
    bool  JUCE_CALLTYPE SystemAudioVolume::setGain (float)   { jassertfalse; return false; }
    bool  JUCE_CALLTYPE SystemAudioVolume::isMuted()         { jassertfalse; return false; }
    bool  JUCE_CALLTYPE SystemAudioVolume::setMuted (bool)   { jassertfalse; return false; }
}
#endif

#include "audio_io/juce_AudioDeviceManager.cpp"
#include "audio_io/juce_AudioIODevice.cpp"
#include "audio_io/juce_AudioIODeviceType.cpp"
#include "midi_io/juce_MidiMessageCollector.cpp"
#include "midi_io/juce_MidiDevices.cpp"
#include "sources/juce_AudioSourcePlayer.cpp"
#include "sources/juce_AudioTransportSource.cpp"

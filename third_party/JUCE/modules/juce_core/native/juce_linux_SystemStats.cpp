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

#if JUCE_BELA
extern "C" int cobalt_thread_mode();
#endif

namespace juce
{

void Logger::outputDebugString (const String& text)
{
    std::cerr << text << std::endl;
}

//==============================================================================
SystemStats::OperatingSystemType SystemStats::getOperatingSystemType()
{
    return Linux;
}

String SystemStats::getOperatingSystemName()
{
    return "WASM";
}

bool SystemStats::isOperatingSystem64Bit()
{
   #if JUCE_64BIT
    return true;
   #else
    //xxx not sure how to find this out?..
    return false;
   #endif
}

//==============================================================================
static String getCpuInfo (const char* key)
{
    return "";
}

String SystemStats::getDeviceDescription()
{
    return "";
}

String SystemStats::getDeviceManufacturer()
{
    return {};
}

String SystemStats::getCpuVendor()
{
    return "";
}

String SystemStats::getCpuModel()
{
    return "?";
}

int SystemStats::getCpuSpeedInMegahertz()
{
    return 4;
}

int SystemStats::getMemorySizeInMegabytes()
{
    return 2048;
}

int SystemStats::getPageSize()
{
    return 4096;
}

//==============================================================================
String SystemStats::getLogonName()
{
    return "root";
}

String SystemStats::getFullUserName()
{
    return getLogonName();
}

String SystemStats::getComputerName()
{
    return "eee_pc";
}

//==============================================================================
uint32 juce_millisecondsSinceStartup() noexcept
{
    return (uint32) 0;
}

int64 Time::getHighResolutionTicks() noexcept
{
    return 0;
}

int64 Time::getHighResolutionTicksPerSecond() noexcept
{
    return 0;  // (microseconds)
}

double Time::getMillisecondCounterHiRes() noexcept
{
    return (double) 0;
}

bool Time::setSystemTimeToThisTime() const
{
    return false;
}

JUCE_API bool JUCE_CALLTYPE juce_isRunningUnderDebugger() noexcept
{
   return false;
}

} // namespace juce

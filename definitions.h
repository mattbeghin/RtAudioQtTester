#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "RtAudio.h"
#include <map>

static const std::map<unsigned long,std::string> s_rtAudioFormatStr = {
    {RTAUDIO_SINT8,"8-bit signed integer"},
    {RTAUDIO_SINT16,"16-bit signed integer"},
    {RTAUDIO_SINT24,"24-bit signed integer"},
    {RTAUDIO_SINT32,"32-bit signed integer"},
    {RTAUDIO_FLOAT32,"32-bits floating point"},
    {RTAUDIO_FLOAT64,"64-bits floating point"}
};

#endif // DEFINITIONS_H

#ifndef GMPI_AUDIO_API_H_INCLUDED
#define GMPI_AUDIO_API_H_INCLUDED

#include "gmpi.h"

enum GMPI_EventType
{
    GMPI_EVENT_TYPE_PIN_SET             = 100, // A parameter has changed value.
    GMPI_EVENT_TYPE_PIN_STREAMING_START = 101, // An input is not silent.
    GMPI_EVENT_TYPE_PIN_STREAMING_STOP  = 102, // An input is silent.
    GMPI_EVENT_TYPE_MIDI                = 103, // A MIDI message.
    GMPI_EVENT_TYPE_GRAPH_START         = 104, // Plugin is about to process the very first sample.
};

typedef struct GMPI_Event
{
    int32_t timeDelta;
    int32_t eventType;
    int32_t parm1;
    int32_t parm2;
    int32_t parm3;
    int32_t parm4;
    char* extraData;
    struct GMPI_Event* next;
} GMPI_Event;

// INTERFACE 'GMPI_IAudioPlugin'
typedef struct GMPI_IAudioPlugin{
    struct GMPI_IAudioPluginMethods* methods;
} GMPI_IAudioPlugin;

typedef struct GMPI_IAudioPluginMethods
{
    // Methods of unknown
    int32_t (*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t (*addRef)(GMPI_IUnknown*);
    int32_t (*release)(GMPI_IUnknown*);

    int32_t (*open)(GMPI_IAudioPlugin*, GMPI_IUnknown* host);
    int32_t (*setBuffer)(GMPI_IAudioPlugin*, int32_t pinId, float* buffer);
    void (*process)(GMPI_IAudioPlugin*, int32_t count, const struct GMPI_Event* events);
} GMPI_IAudioPluginMethods;

// {23835D7E-DCEB-4B08-A9E7-B43F8465939E}
static const GMPI_Guid GMPI_IID_AUDIO_PLUGIN =
{ 0x23835D7E, 0xDCEB, 0x4B08, { 0xA9, 0xE7, 0xB4, 0x3F, 0x84, 0x65, 0x93, 0x9E} };

// INTERFACE 'GMPI_IAudioPluginHost'
typedef struct GMPI_IAudioPluginHost{
    struct GMPI_IAudioPluginHostMethods* methods;
} GMPI_IAudioPluginHost;

typedef struct GMPI_IAudioPluginHostMethods
{
    // Methods of unknown
    int32_t (*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t (*addRef)(GMPI_IUnknown*);
    int32_t (*release)(GMPI_IUnknown*);

    int32_t (*setPin)(GMPI_IAudioPluginHost*, int32_t timestamp, int32_t pinId, int32_t size, const void* data);
    int32_t (*setPinStreaming)(GMPI_IAudioPluginHost*, int32_t timestamp, int32_t pinId, bool isStreaming);
    int32_t (*setLatency)(GMPI_IAudioPluginHost*, int32_t latency);
    int32_t (*sleep)(GMPI_IAudioPluginHost*);
    int32_t (*getBlockSize)(GMPI_IAudioPluginHost*);
    float (*getSampleRate)(GMPI_IAudioPluginHost*);
    int32_t (*getHandle)(GMPI_IAudioPluginHost*);
} GMPI_IAudioPluginHostMethods;

// {87CCD426-71D7-414E-A9A6-5ADCA81C7420}
static const GMPI_Guid GMPI_IID_AUDIO_PLUGIN_HOST =
{ 0x87CCD426, 0x71D7, 0x414E, { 0xA9, 0xA6, 0x5A, 0xDC, 0xA8, 0x1C, 0x74, 0x20} };

#endif

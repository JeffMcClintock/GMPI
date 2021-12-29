#ifndef GMPI_H_INCLUDED
#define GMPI_H_INCLUDED

#include <stdint.h>

enum GMPI_EventType
{
    GMPI_PIN_SET = 100, // A parameter has changed value.
    GMPI_PIN_STREAMING_START = 101, // An input is not silent.
    GMPI_PIN_STREAMING_STOP = 102, // An input is silent.
    GMPI_MIDI = 103, // A MIDI message.
    GMPI_GRAPH_START = 104, // Plugin is about to process the very first sample.
};

typedef struct{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    unsigned char data4[8];
}GMPI_Guid;

typedef struct Event
{
    int32_t TimeDelta;
    int32_t EventType;
    int32_t Parm1;
    int32_t Parm2;
    int32_t Parm3;
    int32_t Parm4;
    const char* ExtraData;
    struct Event* Next;
} Event;

typedef struct GMPI_IUnknown {
    struct GMPI_IUnknownMethods* methods;
} GMPI_IUnknown;

// {00000000-0000-C000-0000-000000000046}
static const GMPI_Guid GMPI_IID_UNKNOWN =
{ 0x00000000, 0x0000, 0xC000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };

typedef struct GMPI_IUnknownMethods
{
    int32_t(*queryInterface)(GMPI_IUnknown*, GMPI_Guid, struct GMPI_Void**);
    int32_t(*addRef)(GMPI_IUnknown*);
    int32_t(*release)(GMPI_IUnknown*);
} GMPI_IUnknownMethods;

typedef struct GMPI_IAudioPlugin {
    struct GMPI_IAudioPluginMethods* methods;
} GMPI_IAudioPlugin;

typedef struct GMPI_IAudioPluginMethods
{
    // TODO: Methods of unknown
    int32_t(*queryInterface)(GMPI_IUnknown*, GMPI_Guid, struct GMPI_Void**);
    int32_t(*addRef)(GMPI_IUnknown*);
    int32_t(*release)(GMPI_IUnknown*);

    int32_t(*setHost)(GMPI_IAudioPlugin*, GMPI_IUnknown);
    int32_t(*open)(GMPI_IAudioPlugin*);
    int32_t(*setBuffer)(GMPI_IAudioPlugin*, int32_t, float*);
    int32_t(*process)(GMPI_IAudioPlugin*, int32_t, struct GMPI_Event*);
} GMPI_IAudioPluginMethods;

// GUID for IMpAudioPlugin
// {23835D7E-DCEB-4B08-A9E7-B43F8465939E}
static const GMPI_Guid GMPI_IID_AUDIO_PLUGIN =
{ 0x23835D7E, 0xDCEB, 0x4B08, { 0xA9, 0xE7, 0xB4, 0x3F, 0x84, 0x65, 0x93, 0x9E} };


typedef struct IGMPI_PluginFactoryMethods
{
    // TODO: Methods of Unknown

    int32_t(*CreateInstance2)(struct GMPI_PluginFactory*, char**, int32_t*, void**);
} IGMPI_PluginFactoryMethods;

typedef struct GMPI_PluginFactory
{
    struct IGMPI_PluginFactoryMethods* methods;
}GMPI_PluginFactory;


#endif
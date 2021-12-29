#ifndef GMPI_H_INCLUDED
#define GMPI_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

enum GMPI_ReturnCode
{
    GMPI_OK = 0,  // Success.
    GMPI_HANDLED = 1,  // Success, no further handing required.
    GMPI_FAIL = -1, // General failure.
    GMPI_UNHANDLED = -1, // Event not handled.
    GMPI_NO_SUPPORT = -2, // Interface not supported.
    GMPI_CANCEL = -3, // Async operation cancelled.
};

enum GMPI_EventType
{
    GMPI_PIN_SET = 100, // A parameter has changed value.
    GMPI_PIN_STREAMING_START = 101, // An input is not silent.
    GMPI_PIN_STREAMING_STOP = 102, // An input is silent.
    GMPI_MIDI = 103, // A MIDI message.
    GMPI_GRAPH_START = 104, // Plugin is about to process the very first sample.
};

typedef struct GMPI_Guid
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} GMPI_Guid;

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

// INTERFACE 'GMPI_IUnknown'
typedef struct GMPI_IUnknown {
    struct GMPI_IUnknownMethods* methods;
} GMPI_IUnknown;

typedef struct GMPI_IUnknownMethods
{
    int32_t(*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t(*addRef)(GMPI_IUnknown*);
    int32_t(*release)(GMPI_IUnknown*);
} GMPI_IUnknownMethods;

// {00000000-0000-C000-0000-000000000046}
static const GMPI_Guid GMPI_IID_UNKNOWN =
{ 0x00000000, 0x0000, 0xC000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };


// INTERFACE 'GMPI_IAudioPlugin'
typedef struct GMPI_IAudioPlugin {
    struct GMPI_IAudioPluginMethods* methods;
} GMPI_IAudioPlugin;

typedef struct GMPI_IAudioPluginMethods
{
    // Methods of unknown
    int32_t(*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t(*addRef)(GMPI_IUnknown*);
    int32_t(*release)(GMPI_IUnknown*);

    int32_t(*setHost)(GMPI_IAudioPlugin*, GMPI_IUnknown* host);
    int32_t(*open)(GMPI_IAudioPlugin*);
    int32_t (*setBuffer)(GMPI_IAudioPlugin*, int32_t pinId, float* buffer);
    int32_t (*process)(GMPI_IAudioPlugin*, int32_t count, const GMPI_Event* events);
} GMPI_IAudioPluginMethods;

// {23835D7E-DCEB-4B08-A9E7-B43F8465939E}
static const GMPI_Guid GMPI_IID_AUDIO_PLUGIN =
{ 0x23835D7E, 0xDCEB, 0x4B08, { 0xA9, 0xE7, 0xB4, 0x3F, 0x84, 0x65, 0x93, 0x9E} };


// INTERFACE 'GMPI_IAudioPluginHost'
typedef struct GMPI_IAudioPluginHost {
    struct GMPI_IAudioPluginHostMethods* methods;
} GMPI_IAudioPluginHost;

typedef struct GMPI_IAudioPluginHostMethods
{
    // Methods of unknown
    int32_t(*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t(*addRef)(GMPI_IUnknown*);
    int32_t(*release)(GMPI_IUnknown*);

    int32_t(*setPin)(GMPI_IAudioPluginHost*, int32_t timestamp, int32_t pinId, int32_t size, const void* data);
    int32_t (*setPinStreaming)(GMPI_IAudioPluginHost*, int32_t timestamp, int32_t pinId, bool isStreaming);
    int32_t(*setLatency)(GMPI_IAudioPluginHost*, int32_t latency);
    int32_t(*sleep)(GMPI_IAudioPluginHost*);
    int32_t(*getBlockSize)(GMPI_IAudioPluginHost*);
    int32_t(*getSampleRate)(GMPI_IAudioPluginHost*);
    int32_t(*getHandle)(GMPI_IAudioPluginHost*);
} GMPI_IAudioPluginHostMethods;

// {87CCD426-71D7-414E-A9A6-5ADCA81C7420}
static const GMPI_Guid GMPI_IID_AUDIO_PLUGIN_HOST =
{ 0x87CCD426, 0x71D7, 0x414E, { 0xA9, 0xA6, 0x5A, 0xDC, 0xA8, 0x1C, 0x74, 0x20} };


// INTERFACE 'GMPI_IString'
typedef struct GMPI_IString{
    struct GMPI_IStringMethods* methods;
} GMPI_IString;

typedef struct GMPI_IStringMethods
{
    int32_t (*setData)(GMPI_IString*, const char* data, int32_t size);
    int32_t (*getSize)(GMPI_IString*);
    int32_t (*getData)(GMPI_IString*);
} GMPI_IStringMethods;

// {AB8FFB21-44FF-42B7-8885-29431399E7E4}
static const GMPI_Guid GMPI_IID_STRING =
{ 0xAB8FFB21, 0x44FF, 0x42B7, { 0x88, 0x85, 0x29, 0x43, 0x13, 0x99, 0xE7, 0xE4} };


// INTERFACE 'GMPI_IStringTest'
typedef struct GMPI_IStringTest{
    struct GMPI_IStringTestMethods* methods;
} GMPI_IStringTest;

typedef struct GMPI_IStringTestMethods
{
    int32_t (*setData)(GMPI_IStringTest*, const char* data, int32_t size);
    int32_t (*getData)(GMPI_IStringTest*, char** returnData, int32_t* returnSize);
} GMPI_IStringTestMethods;

// {FB8FFB21-44FF-42B7-8885-29431399E7E4}
static const GMPI_Guid GMPI_IID_STRING_TEST =
{ 0xFB8FFB21, 0x44FF, 0x42B7, { 0x88, 0x85, 0x29, 0x43, 0x13, 0x99, 0xE7, 0xE4} };


// INTERFACE 'GMPI_IPluginFactory'
typedef struct GMPI_IPluginFactory {
    struct GMPI_IPluginFactoryMethods* methods;
} GMPI_IPluginFactory;

typedef struct GMPI_IPluginFactoryMethods
{
    // Methods of unknown
    int32_t(*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t(*addRef)(GMPI_IUnknown*);
    int32_t(*release)(GMPI_IUnknown*);

    int32_t (*createInstance)(GMPI_IPluginFactory*, const char* id, int32_t subtype, void** returnInterface);
} GMPI_IPluginFactoryMethods;

// {31DC1CD9-6BDF-412A-B758-B2E5CD1D8870}
static const GMPI_Guid GMPI_IID_PLUGIN_FACTORY =
{ 0x31DC1CD9, 0x6BDF, 0x412A, { 0xB7, 0x58, 0xB2, 0xE5, 0xCD, 0x1D, 0x88, 0x70} };


#endif

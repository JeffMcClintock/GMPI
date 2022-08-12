#ifndef GMPI_H_INCLUDED
#define GMPI_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

enum GMPI_ReturnCode
{
    GMPI_RETURN_CODE_OK         = 0,  // Success.
    GMPI_RETURN_CODE_HANDLED    = 1,  // Success, no further handing required.
    GMPI_RETURN_CODE_FAIL       = -1, // General failure.
    GMPI_RETURN_CODE_UNHANDLED  = -1, // Event not handled.
    GMPI_RETURN_CODE_NO_SUPPORT = -2, // Interface not supported.
    GMPI_RETURN_CODE_CANCEL     = -3, // Async operation cancelled.
};

enum GMPI_PluginSubtype
{
    GMPI_PLUGIN_SUBTYPE_AUDIO      = 0, // An audio processor object.
    GMPI_PLUGIN_SUBTYPE_EDITOR     = 2, // A graphical editor object.
    GMPI_PLUGIN_SUBTYPE_CONTROLLER = 4, // A controller object.
};

enum GMPI_PinDirection
{
    GMPI_PIN_DIRECTION_IN,
    GMPI_PIN_DIRECTION_OUT,
};

enum GMPI_PinDatatype
{
    GMPI_PIN_DATATYPE_ENUM,
    GMPI_PIN_DATATYPE_STRING,
    GMPI_PIN_DATATYPE_MIDI,
    GMPI_PIN_DATATYPE_FLOAT64,
    GMPI_PIN_DATATYPE_BOOL,
    GMPI_PIN_DATATYPE_AUDIO,
    GMPI_PIN_DATATYPE_FLOAT32,
    GMPI_PIN_DATATYPE_INT32 = 8,
    GMPI_PIN_DATATYPE_INT64,
    GMPI_PIN_DATATYPE_BLOB,
};

typedef struct GMPI_Guid
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} GMPI_Guid;

// INTERFACE 'GMPI_IUnknown'
typedef struct GMPI_IUnknown{
    struct GMPI_IUnknownMethods* methods;
} GMPI_IUnknown;

typedef struct GMPI_IUnknownMethods
{
    int32_t (*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t (*addRef)(GMPI_IUnknown*);
    int32_t (*release)(GMPI_IUnknown*);
} GMPI_IUnknownMethods;

// {00000000-0000-C000-0000-000000000046}
static const GMPI_Guid GMPI_IID_UNKNOWN =
{ 0x00000000, 0x0000, 0xC000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };

// INTERFACE 'GMPI_IString'
typedef struct GMPI_IString{
    struct GMPI_IStringMethods* methods;
} GMPI_IString;

typedef struct GMPI_IStringMethods
{
    // Methods of unknown
    int32_t (*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t (*addRef)(GMPI_IUnknown*);
    int32_t (*release)(GMPI_IUnknown*);

    int32_t (*setData)(GMPI_IString*, const char* data, int32_t size);
    int32_t (*getSize)(GMPI_IString*);
    const char* (*getData)(GMPI_IString*);
} GMPI_IStringMethods;

// {AB8FFB21-44FF-42B7-8885-29431399E7E4}
static const GMPI_Guid GMPI_IID_STRING =
{ 0xAB8FFB21, 0x44FF, 0x42B7, { 0x88, 0x85, 0x29, 0x43, 0x13, 0x99, 0xE7, 0xE4} };

// INTERFACE 'GMPI_IPluginFactory'
typedef struct GMPI_IPluginFactory{
    struct GMPI_IPluginFactoryMethods* methods;
} GMPI_IPluginFactory;

typedef struct GMPI_IPluginFactoryMethods
{
    // Methods of unknown
    int32_t (*queryInterface)(GMPI_IUnknown*, const GMPI_Guid* iid, void** returnInterface);
    int32_t (*addRef)(GMPI_IUnknown*);
    int32_t (*release)(GMPI_IUnknown*);

    int32_t (*createInstance)(GMPI_IPluginFactory*, const char* id, int32_t subtype, void** returnInterface);
    int32_t (*getPluginInformation)(GMPI_IPluginFactory*, int32_t index, GMPI_IString* returnXml);
} GMPI_IPluginFactoryMethods;

// {31DC1CD9-6BDF-412A-B758-B2E5CD1D8870}
static const GMPI_Guid GMPI_IID_PLUGIN_FACTORY =
{ 0x31DC1CD9, 0x6BDF, 0x412A, { 0xB7, 0x58, 0xB2, 0xE5, 0xCD, 0x1D, 0x88, 0x70} };

#endif

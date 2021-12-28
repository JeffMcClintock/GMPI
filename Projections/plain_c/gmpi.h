#ifndef GMPI_H_INCLUDED
#define GMPI_H_INCLUDED

struct Event
{
    int32_t TimeDelta;
    int32_t EventType;
    int32_t Parm1;
    int32_t Parm2;
    int32_t Parm3;
    int32_t Parm4;
    const char* ExtraData;
    Event* Next;
};

typedef struct IGMPI_UnknownMethods
{
    int32_t(*QueryInterface)(GMPI_Unknown*, typeerror, typeerror);
    int32_t(*AddRef)(GMPI_Unknown*);
    int32_t(*Release)(GMPI_Unknown*);
} IGMPI_UnknownMethods;

typedef struct GMPI_Unknown {
    struct IGMPI_UnknownMethods* methods;
};

typedef struct IGMPI_AudioPluginMethods
{
    // TODO: Methods of Unknown

    int32_t(*SetHost)(GMPI_AudioPlugin*, typeerror);
    int32_t(*Open)(GMPI_AudioPlugin*);
    int32_t(*Setbuffer)(GMPI_AudioPlugin*, int32_t, float*);
    int32_t(*Process)(GMPI_AudioPlugin*, int32_t, event*);
} IGMPI_AudioPluginMethods;

typedef struct GMPI_AudioPlugin {
    struct IGMPI_AudioPluginMethods* methods;
};


#endif
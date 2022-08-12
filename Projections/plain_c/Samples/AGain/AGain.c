#include <stdlib.h>
#include "gmpi_audio.h"

typedef struct
{
	const GMPI_IAudioPluginMethods* methods;

	float* in1;
	float* in2;
	float* out1;
	float* out2;
	float gain;

	int refCount;
	GMPI_IAudioPluginHost* host;
} CGain;

void CGain_Process_Events(CGain* ths, const GMPI_Event* events);

void CGain_Process(GMPI_IAudioPlugin* ths, int32_t count, const GMPI_Event* events)
{
	CGain* plugin = (CGain*)ths;

	CGain_Process_Events(plugin, events);

	const float* in1 = plugin->in1;
	const float* in2 = plugin->in2;
	float* out1 = plugin->out1;
	float* out2 = plugin->out2;

	for (int s = 0; s < count; ++s)
	{
		*out1++ = *in1++ * plugin->gain;
		*out2++ = *in2++ * plugin->gain;
	}
}

void CGain_OnGraphStart(CGain* ths)
{
	ths->host->methods->setPinStreaming(ths->host, 0, 2, true);
	ths->host->methods->setPinStreaming(ths->host, 0, 3, true);
}

void CGain_Process_Events(CGain* ths, const GMPI_Event* events)
{
	for (const GMPI_Event* e = events; e; e = e->next)
	{
		switch (e->eventType)
		{
		case GMPI_EVENT_TYPE_GRAPH_START:
			CGain_OnGraphStart(ths);
			break;
		}
	}
}

int32_t CGain_open(GMPI_IAudioPlugin* ths, GMPI_IUnknown* host)
{
	CGain* plugin = (CGain*)ths;

	return host->methods->queryInterface(host, &GMPI_IID_AUDIO_PLUGIN_HOST, &(plugin->host));
}

int32_t CGain_Setbuffer(GMPI_IAudioPlugin* ths, int32_t pin, float* buffer)
{
	CGain* plugin = (CGain*) ths;

	switch (pin)
	{
	case 0:
		plugin->in1 = buffer;
		break;
	case 1:
		plugin->in2 = buffer;
		break;
	case 2:
		plugin->out1 = buffer;
		break;
	case 3:
		plugin->out2 = buffer;
		break;
	}
	return GMPI_RETURN_CODE_OK;
}

int32_t CGain_QueryInterface(GMPI_IUnknown* u, const GMPI_Guid* guid, void** ret)
{
	*ret = 0;

	if (memcmp(guid, &GMPI_IID_AUDIO_PLUGIN, sizeof(*guid)) == 0
		|| memcmp(guid, &GMPI_IID_UNKNOWN, sizeof(*guid)) == 0)
	{
		*ret = u;
		u->methods->addRef(u);
	}

	return GMPI_RETURN_CODE_OK;
}

int32_t CGain_AddRef(GMPI_IUnknown* u)
{
	CGain* plugin = (CGain*)u;
	return ++(plugin->refCount);
}

int32_t CGain_Release(GMPI_IUnknown* u)
{
	CGain* plugin = (CGain*)u;
	const int refCount = --(plugin->refCount);
	if (refCount == 0)
	{
		free(u);
	}

	return refCount;
}

static const GMPI_IAudioPluginMethods CGainMethods =
{
	CGain_QueryInterface,
	CGain_AddRef,
	CGain_Release,

	CGain_open,
	CGain_Setbuffer,
	CGain_Process
};

void* createCGain()
{
	CGain* plugin;
	plugin = malloc(sizeof(*plugin));

	plugin->gain = 0.5f;
	plugin->refCount = 1;
	plugin->methods = &CGainMethods;

	return plugin;
}

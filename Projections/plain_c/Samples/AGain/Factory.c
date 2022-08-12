#include <stdlib.h>
#include "gmpi.h"
#include "AGain.h"

typedef struct
{
	const GMPI_IPluginFactoryMethods* methods;
	int refCount;
} Factory;

int32_t Factory_createInstance(GMPI_IPluginFactory* ths, const char* id, int32_t subtype, void** returnInterface)
{
	if (subtype != GMPI_PLUGIN_SUBTYPE_AUDIO)
		return GMPI_RETURN_CODE_NO_SUPPORT;

	*returnInterface = createCGain();
	return GMPI_RETURN_CODE_OK;
}

int32_t Factory_queryInterface(GMPI_IUnknown* u, const GMPI_Guid* guid, void** ret)
{
	*ret = 0;

	if (memcmp(guid, &GMPI_IID_PLUGIN_FACTORY, sizeof(*guid)) == 0
		|| memcmp(guid, &GMPI_IID_UNKNOWN, sizeof(*guid)) == 0)
	{
		*ret = u;
		u->methods->addRef(u);
		return GMPI_RETURN_CODE_OK;
	}

	return GMPI_RETURN_CODE_NO_SUPPORT;
}

int32_t Factory_addRef(GMPI_IUnknown* u)
{
	Factory* plugin = (Factory*)u;
	return ++(plugin->refCount);
}

int32_t Factory_release(GMPI_IUnknown* u)
{
	Factory* plugin = (Factory*)u;
	const int refCount = --(plugin->refCount);
	if (refCount == 0)
	{
		free(u);
	}

	return refCount;
}

static const GMPI_IPluginFactoryMethods Factory_Methods =
{
	Factory_queryInterface,
	Factory_addRef,
	Factory_release,

	Factory_createInstance,
};

#ifdef _WIN32
__declspec (dllexport)
#else

#if defined (__GNUC__)
__attribute__ ((visibility ("default")))
#endif

#endif

int32_t MP_GetFactory(void** returnInterface)
{
	Factory* factory;
	factory = malloc(sizeof(*factory));

	factory->refCount = 0;
	factory->methods = &Factory_Methods;

	return factory->methods->queryInterface((GMPI_IUnknown*) factory, &GMPI_IID_UNKNOWN, returnInterface);
}

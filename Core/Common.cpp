/*
  GMPI - Generalized Music Plugin Interface specification.
  Copyright 2007-2023 Jeff McClintock.

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define GMPI_SDK_REVISION 9000 // 0.90
/* Version History
*    12/25/2021 - V0.90 : Draft release
*/

#include <map>
#include <vector>
#include <string>
#include <cstring> // std::strlen
#include <cassert>
#include "Common.h"
#include "RefCountMacros.h"

using namespace gmpi;
using namespace gmpi::api;

//---------------FACTORY --------------------
#ifndef GMPI_DISABLE_FACTORY // or just exclude this file.

// MpFactory - a singleton object.  The plugin registers it's ID with the factory.

class MpFactory : public IPluginFactory
{
public:
    MpFactory() {}
    virtual ~MpFactory() {}

    // IMpPluginFactory methods
    ReturnCode createInstance(
        const char* uniqueId,
        PluginSubtype subType,
        void** returnInterface) override;

    ReturnCode getPluginInformation(int32_t index, IString* returnXml) override;

    ReturnCode RegisterPlugin(const char* uniqueId, PluginSubtype subType, CreatePluginPtr create);
    ReturnCode RegisterPluginWithXml(PluginSubtype subType, const char* xml, CreatePluginPtr create);

    // IUnknown methods
    GMPI_QUERYINTERFACE_METHOD(IPluginFactory);
    GMPI_REFCOUNT_NO_DELETE

private:
    // a map of all registered IIDs.
    std::map<std::pair<PluginSubtype, std::string>, CreatePluginPtr> pluginMap;
    std::vector<std::string> xmls;
};

MpFactory& Factory()
{
    static MpFactory theFactory;
    return theFactory;
}

// This is the DLL's main entry point.  It returns the factory.
extern "C"

#ifdef _WIN32
    __declspec (dllexport)
#else
#if defined (__GNUC__)
    __attribute__ ((visibility ("default")))
#endif
#endif

ReturnCode MP_GetFactory(void** returnInterface)
{
    if (!returnInterface)
        return ReturnCode::Fail;

    // call queryInterface() to keep refcounting in sync
    return Factory().queryInterface(&IUnknown::guid, returnInterface);
}

namespace gmpi
{
	// register a plugin component with the factory
    ReturnCode RegisterPlugin(PluginSubtype subType, const char* uniqueId, CreatePluginPtr create)
	{
		return Factory().RegisterPlugin(uniqueId, subType, create);
	}
    ReturnCode RegisterPluginWithXml(PluginSubtype subType, const char* xml, CreatePluginPtr create)
	{
		return Factory().RegisterPluginWithXml(subType, xml, create);
	}
}

// Factory methods
ReturnCode MpFactory::RegisterPlugin(const char* uniqueId, PluginSubtype subType, CreatePluginPtr create)
{
    if (!create || !uniqueId)
        return ReturnCode::Fail;

    // already registered this plugin?
    assert(pluginMap.find({ subType, uniqueId }) == pluginMap.end());

	pluginMap[{ subType, uniqueId }] = create;

    return ReturnCode::Ok;
}

ReturnCode MpFactory::RegisterPluginWithXml(PluginSubtype subType, const char* xml, CreatePluginPtr create)
{
    if (!create || !xml)
        return ReturnCode::Fail;

	std::string xmlstr{xml};

	size_t p{};
	for (auto s : { "<Plugin", " id", "\"" })
    {
		p = xmlstr.find(s, p) + strlen(s);
    }

	const auto p2 = xmlstr.find("\"", p);

	if( p == std::string::npos || p2 == std::string::npos)
        return ReturnCode::Fail;

	const auto uniqueId = xmlstr.substr(p, p2 - p);

    // already registered this plugin?
    assert(pluginMap.find({ subType, uniqueId }) == pluginMap.end());

    pluginMap[{ subType, uniqueId }] = create;

	xmls.push_back(xmlstr);

    return ReturnCode::Ok;
}

ReturnCode MpFactory::createInstance(const char* uniqueId, PluginSubtype subType, void** returnInterface)
{
    if (!returnInterface || !uniqueId)
        return ReturnCode::Fail;

    *returnInterface = nullptr; // default to nullptr on failure

    // search m_pluginMap for the requested ID
    auto it = pluginMap.find({ subType, uniqueId });
    if (it == pluginMap.end())
    {
        return ReturnCode::NoSupport;
    }

    auto create = it->second;
    if (!create)
    {
        return ReturnCode::NoSupport;
    }

    try
    {
        auto m = create();
        *returnInterface = m;
#ifdef _DEBUG
        {
            m->addRef();
            const auto refcount = m->release();
            assert(refcount == 1);
        }
#endif
    }
    // the new function will throw a std::bad_alloc exception if the memory allocation fails.
    // the constructor will throw a char* if host don't support required interfaces.
    catch (...)
    {
        return ReturnCode::Fail;
    }

    return ReturnCode::Ok;
}

ReturnCode MpFactory::getPluginInformation(int32_t index, IString* returnXml)
{
    if (!returnXml)
        return ReturnCode::Fail;

    if (index < 0 || static_cast<size_t>(index) >= xmls.size())
    {
        returnXml->setData(nullptr, 0);
        return ReturnCode::Fail;
    }

    returnXml->setData(xmls[static_cast<size_t>(index)].data(),
                       static_cast<int32_t>(xmls[static_cast<size_t>(index)].size()));
    return ReturnCode::Ok;
}

#endif // GMPI_DISABLE_FACTORY

#pragma once
#include "./EmbeddedFile.h"
#include "GmpiSdkCommon.h"
//#include "RefCountMacros.h"
//#include "Common.h"
//#include "GmpiApiCommon.h"

namespace synthedit
{

class EmbeddedFileHostWrapper
{
	gmpi::shared_ptr<IEmbeddedFileSupport> host;

public:
	gmpi::ReturnCode Init(gmpi::api::IUnknown* phost)
	{
		host = gmpi::as<synthedit::IEmbeddedFileSupport>(phost);
		return host ? gmpi::ReturnCode::Ok : gmpi::ReturnCode::NoSupport;
	}

	// IEmbeddedFileSupport
	std::string resolveFilename(std::string filename)
	{
		gmpi::ReturnString fullFilename;
		host->resolveFilename(filename.c_str(), &fullFilename);
		return fullFilename.c_str();
	}
#if 0 // TODO, implement class 'UriFile'
	UriFile AudioPluginHostWrapper::openUri(std::string uri)
	{
		gmpi_sdk::mp_shared_ptr<gmpi::IEmbeddedFileSupport> dspHost;
		Get()->queryInterface(gmpi::MP_IID_HOST_EMBEDDED_FILE_SUPPORT, dspHost.asIMpUnknownPtr());
		assert(dspHost); // new way

		gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> obj;
		dspHost->openUri(uri.c_str(), &obj.get());

		return { obj };
	}
#endif
};

} //namespace synthedit

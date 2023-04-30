#pragma once
/*
#include "Extensions/EmbeddedFile.h"
*/

// SynthEdit-specific.
namespace synthedit
{
// extension to GMPI to provide support for loading files from the plugins resources (be they embedded or file-based).
// Corrects error in IMpHost that there is a memory leak, due to having no way to free the file object, and no way to query file object for updated interfaces.
struct DECLSPEC_NOVTABLE IEmbeddedFileSupport : public gmpi::api::IUnknown
{
public:
	// Determine file's location depending on host application's conventions. // e.g. "bell.wav" -> "C:/My Documents/bell.wav"
	virtual gmpi::ReturnCode resolveFilename(const char* fileName, gmpi::api::IString* returnFullUri) = 0;

	// open a file, usually returns a IProtectedFile2 interface.
	virtual gmpi::ReturnCode openUri(const char* fullUri, gmpi::api::IUnknown** returnStream) = 0;

	// {B486F4DE-9010-4AA0-9D0C-DCD9F8879257}
	inline static const gmpi::api::Guid guid =
	{ 0xb486f4de, 0x9010, 0x4aa0, { 0x9d, 0x0c, 0xdc, 0xd9, 0xf8, 0x87, 0x92, 0x57 } };
};

} //namespace synthedit

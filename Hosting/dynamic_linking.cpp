#include <assert.h>
#include "dynamic_linking.h"

// Provide a cross-platform loading of dlls.

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "windows.h"
#else
#include "unicode_conversion.h"
using namespace wrapper::JmUnicodeConversions;
#endif
#if __APPLE__
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace wrapper
{
namespace gmpi_dynamic_linking
{

#if defined(_WIN32)
	typedef HINSTANCE MP_DllHandle;
#else
	typedef void* MP_DllHandle;
#endif

    int32_t MP_DllLoad(DLL_HANDLE* dll_handle, const char* dll_filename)
    {
        *dll_handle = {};

#if defined( _WIN32)
        *dll_handle = (DLL_HANDLE)LoadLibraryA(dll_filename);
#else
#if __APPLE__
        * dll_handle = (DLL_HANDLE)dlopen(dll_filename, 0);
#endif
#endif
        return *dll_handle == 0;
    }

	int32_t MP_DllLoad(DLL_HANDLE* dll_handle, const wchar_t* dll_filename)
	{
        *dll_handle = {};

#if defined( _WIN32)
		*dll_handle = (DLL_HANDLE) LoadLibraryW(dll_filename);
#else
#if __APPLE__
        *dll_handle = (DLL_HANDLE) dlopen(WStringToUtf8(dll_filename).c_str(), 0);
#endif
#endif
        return *dll_handle == 0;
	}

	int32_t MP_DllUnload(DLL_HANDLE dll_handle)
	{
		int32_t r = 0;
		if (dll_handle)
		{
#if defined( _WIN32)
			r = FreeLibrary((HMODULE)dll_handle);
#else
#if __APPLE__
            r = dlclose((MP_DllHandle)dll_handle);
#endif
#endif
		}
		return r == 0;
	}

	bool MP_DllSymbol(DLL_HANDLE dll_handle, const char* symbol_name, void** returnFunction)
	{
		*returnFunction = nullptr;

#if defined( _WIN32)
		*returnFunction = (void*) GetProcAddress((HMODULE)dll_handle, symbol_name);
#else
#if __APPLE__
        *returnFunction = dlsym((MP_DllHandle) dll_handle, symbol_name);
#endif
#endif
		return *returnFunction == nullptr;
	}

    // Provide a static function to allow GetModuleHandleExA() to find dll name.
    void localFuncWithUNlikelyName3456()
    {
    }

#if defined(_WIN32)
	int32_t MP_GetDllHandle(DLL_HANDLE* returnDllHandle)
	{
		HMODULE hmodule = 0;
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&localFuncWithUNlikelyName3456, &hmodule);
		*returnDllHandle = ( DLL_HANDLE) hmodule;
		return 1;
	}

	std::wstring MP_GetDllFilename()
	{
		DLL_HANDLE hmodule = 0;
		MP_GetDllHandle(&hmodule);

		wchar_t full_path[MAX_PATH] = L"";
		GetModuleFileNameW((HMODULE)hmodule, full_path, std::size(full_path));
		return std::wstring(full_path);
	}
    
#else
    // Mac
// TODO!!! ReleaseDllHandle(DLL_HANDLE* returnDllHandle)
    int32_t MP_GetDllHandle(DLL_HANDLE* returnDllHandle)
    {
        *returnDllHandle = {};
#if __APPLE__
        Dl_info info;
        if (dladdr ((const void*)MP_GetDllHandle, &info))
        {
            if (info.dli_fname)
            {
                std::string name;
                name.assign (info.dli_fname);
                for (int i = 0; i < 3; i++)
                {
                    auto p = name.find_last_of ('/');
                    if (p == std::string::npos)
                    {
                        fprintf (stdout, "Could not determine bundle location.\n");
                        return 0; // unexpected
                    }
                    name = name.substr(0, p);
                }
                CFURLRef bundleUrl = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*)name.c_str(), name.length (), true);
                if (bundleUrl)
                {
                    const auto rBundleRef = CFBundleCreate (0, bundleUrl);
                    CFRelease (bundleUrl);
                    
                    *returnDllHandle = (DLL_HANDLE) rBundleRef;
                    return 1;
                }
            }
        }
#endif
        return 0;
    }

    std::wstring MP_GetDllFilename()
    {
#if __APPLE__
        Dl_info info;
        int rv = dladdr((void *)&localFuncWithUNlikelyName3456, &info);
        assert(rv != 0);

        return Utf8ToWstring(info.dli_fname);
#else
        return {}; // TODO Linux
#endif
    }
#endif
}
}
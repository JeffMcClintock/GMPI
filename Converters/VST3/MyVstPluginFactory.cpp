#include "MyVstPluginFactory.h"
#include "pluginterfaces/base/funknown.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "adelayprocessor.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "adelaycontroller.h"
#include "tinyxml2/tinyxml2.h"
#include "public.sdk/source/vst/vstcomponent.h"
//#include "UgDatabase.h"
#include "it_enum_list.h"
#include "xp_dynamic_linking.h"
#include "BundleInfo.h"
#include "FileFinder.h"
#include "FileFinder.h"
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"

#if defined( _WIN32 )
extern HINSTANCE ghInst;
#else
	#include <dlfcn.h>
	#include <CoreFoundation/CoreFoundation.h>

/*
// Hacky, access functions meant only for VSTGUI.
namespace VSTGUI
{
    static void CreateVSTGUIBundleRef();
    static void ReleaseVSTGUIBundleRef();
}
*/

#if 0
// Copied from: public.sdk/source/vst/vstguieditor.cpp
//void* gBundleRef = 0;
//static int openCount = 0;
//CFBundleRef getBundleRef(){return (CFBundleRef) gBundleRef;};
//------------------------------------------------------------------------
CFBundleRef CreatePluginBundleRef ()
{
    CFBundleRef rBundleRef = 0;
    
	Dl_info info;
	if (dladdr ((const void*)CreatePluginBundleRef, &info))
	{
		if (info.dli_fname)
		{
			Steinberg::String name;
			name.assign (info.dli_fname);
			for (int i = 0; i < 3; i++)
			{
				int delPos = name.findLast ('/');
				if (delPos == -1)
				{
					fprintf (stdout, "Could not determine bundle location.\n");
					return 0; // unexpected
				}
				name.remove (delPos, name.length () - delPos);
			}
			CFURLRef bundleUrl = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*)name.text8 (), name.length (), true);
			if (bundleUrl)
			{
				rBundleRef = CFBundleCreate (0, bundleUrl);
				CFRelease (bundleUrl);
			}
		}
	}
    
    return rBundleRef;
}

void ReleasePluginBundleRef (CFBundleRef bundleRef)
{
	CFRelease (bundleRef);
}
#endif
/*
//------------------------------------------------------------------------
void ReleaseVSTGUIBundleRef ()
{
	openCount--;
	if (gBundleRef)
		CFRelease (gBundleRef);
	if (openCount == 0)
		gBundleRef = 0;
}
 */
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;


EXPORT_FACTORY IPluginFactory* PLUGIN_API GetPluginFactory ()
{
	return MyVstPluginFactory::GetInstance();
}

MyVstPluginFactory* MyVstPluginFactory::GetInstance()
{
	static MyVstPluginFactory singleton;
	return &singleton;
}

MyVstPluginFactory::MyVstPluginFactory()
{
}

MyVstPluginFactory::~MyVstPluginFactory()
{
}

/** Fill a PFactoryInfo structure with information about the Plug-in vendor. */
tresult MyVstPluginFactory::getFactoryInfo (PFactoryInfo* info)
{
	initialize();

	strncpy8 (info->vendor, vendorName_.c_str(), PFactoryInfo::kNameSize);
	strncpy8 (info->url, vendorUrl_.c_str(), PFactoryInfo::kURLSize);
	strncpy8 (info->email, vendorEmail_.c_str(), PFactoryInfo::kEmailSize);
	info->flags = PFactoryInfo::kUnicode;
	return kResultOk;
}

/** Returns the number of exported classes by this factory. 
If you are using the CPluginFactory implementation provided by the SDK, it returns the number of classes you registered with CPluginFactory::registerClass. */
int32 MyVstPluginFactory::countClasses ()
{
	initialize();
	return plugins.size() * 2;
}
	
/** Fill a PClassInfo structure with information about the class at the specified index. */
tresult MyVstPluginFactory::getClassInfo (int32 index, PClassInfo* info)
{
	initialize();

	if (index < 0 || index > plugins.size() * 2)
	{
		return kInvalidArgument;
	}

	const int pluginIndex = index / 2;
	const int classIndex = index % 2;

	switch(classIndex)
	{
	case 0:
		strncpy8 (info->category, kVstAudioEffectClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &(pluginInfo_.processorId.toTUID()), sizeof(TUID));
		break;
	case 1:
		strncpy8 (info->category, kVstComponentControllerClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &(pluginInfo_.controllerId.toTUID()), sizeof(TUID));
		break;
	}

	info->cardinality = PClassInfo::kManyInstances;

	strncpy8 (info->name, plugins[pluginIndex].name.c_str(), PClassInfo::kNameSize );

	return kResultOk;
}

/** Returns the class info (version 2) for a given index. */
tresult MyVstPluginFactory::getClassInfo2 (int32 index, PClassInfo2* info)
{
	initialize();

	if (index < 0 || index > plugins.size() * 2)
	{
		return kInvalidArgument;
	}

	const int pluginIndex = index / 2;
	const int classIndex = index % 2;

	info->cardinality = PClassInfo::kManyInstances;

	strncpy8 (info->name, plugins[pluginIndex].name.c_str(), PClassInfo::kNameSize );
	strncpy8 (info->sdkVersion, kVstVersionString, PClassInfo2::kVersionSize );
	strncpy8 (info->vendor, vendorName_.c_str(), PClassInfo2::kVendorSize );
	strncpy8 (info->version, pluginInfo_.version_.c_str(), PClassInfo2::kVersionSize );

	switch(classIndex)
	{
	case 0:
		info->classFlags = Vst::kDistributable;
		strncpy8 (info->subCategories, pluginInfo_.subCategories_.c_str(), PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstAudioEffectClass, PClassInfo::kCategorySize );
		memcpy (info->cid, &(pluginInfo_.processorId.toTUID()), sizeof (TUID));
		break;
	case 1:
		info->classFlags = 0;
		strncpy8 (info->subCategories, "", PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstComponentControllerClass, PClassInfo::kCategorySize );
		memcpy (info->cid, &(pluginInfo_.controllerId.toTUID()), sizeof (TUID));
		break;
	}

	return kResultOk;
}

int32_t MyVstPluginFactory::getVst2Id(int32_t index) // not used for historic reasons. 32-bit comptible.
{
	initialize();

	if (index != 0)
	{
		return kInvalidArgument;
	}

	return backwardCompatible4charId;
}

int32_t MyVstPluginFactory::getVst2Id64(int32_t pluginIndex) // generated from hash of GUID. Not compatible w 32-bit VSTs.
{
	initialize();

	if (pluginIndex != 0)
	{
		return kInvalidArgument;
	}

	Steinberg::PClassInfoW info;
	getClassInfoUnicode(pluginIndex, &info);

	// generate an VST2 ID from VST3 GUIID.
	// see also CContainer::VstUniqueID()
	unsigned char vst2ID[4] = { 'A', 'A', 'A', 'A' };
	int i2 = 0;
	for (int i = 0; i < sizeof(info.cid); ++i)
	{
		vst2ID[i2] = vst2ID[i2] + info.cid[i];
		i2 = (i2 + 1) & 0x03;
	}

	return *((int32_t*)vst2ID);
}

/** Returns the unicode class info for a given index. */
tresult MyVstPluginFactory::getClassInfoUnicode (int32 index, PClassInfoW* info)
{
	initialize();
 
	if( index < 0 || index > plugins.size() * 2 )
	{
		return kInvalidArgument;
	}

	const int pluginIndex = index / 2;
	const int classIndex = index % 2;

	// todo unique processor and control GUIDs based on plugin unique-id
	switch(classIndex)
	{
	case 0:
		info->classFlags = Vst::kDistributable;
		strncpy8 (info->subCategories, pluginInfo_.subCategories_.c_str(), PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstAudioEffectClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &(pluginInfo_.processorId.toTUID()), sizeof(TUID));
		break;
	case 1:
		info->classFlags = 0;
		strncpy8 (info->subCategories, "", PClassInfo2::kSubCategoriesSize );
		strncpy8 (info->category, kVstComponentControllerClass, PClassInfo::kCategorySize );
		memcpy(info->cid, &(pluginInfo_.controllerId.toTUID()), sizeof(TUID));
		break;
	}

	info->cardinality = PClassInfo::kManyInstances;

	str8ToStr16 (info->sdkVersion, kVstVersionString, PClassInfo2::kVersionSize );
	str8ToStr16 (info->version, pluginInfo_.version_.c_str(), PClassInfo2::kVersionSize );
	str8ToStr16 (info->vendor, vendorName_.c_str(), PClassInfo2::kVendorSize );
	str8ToStr16 (info->name, plugins[pluginIndex].name.c_str(), PClassInfo::kNameSize );

	return kResultOk;
}
	
		/** Recieves information about host*/
tresult MyVstPluginFactory::setHostContext (FUnknown* context)
{
	return kNotImplemented;
}

/** Create a new class instance. */
tresult MyVstPluginFactory::createInstance (FIDString cid, FIDString iid, void** obj)
{
	FUID classId = (FUID) *(TUID*)cid;
	FUID interfaceId = (FUID) *(TUID*)iid;

	/* testing
	{
		GetApp()->SeMessageBox( L"Yo", L"Yo", 0);
		char txt[65];
		classId.toString(txt);
		char txt2[65];
		interfaceId.toString(txt2);

		_RPT2(_CRT_WARN, "MyVstPluginFactory::createInstance(%s,%s)", txt, txt2);
	}
	*/

	initialize();

	FUnknown* instance;
	if (interfaceId == IComponent::iid || classId == pluginInfo_.processorId)
	{
		auto i = new SeProcessor();
/* Now done by detecting MIDI input
		if( pluginInfo_.subCategories_.find( "Instrument" ) != std::string::npos )
		{
			i->setSynth();
		}
*/
		i->setControllerClass (pluginInfo_.controllerId); // associate with controller.
		instance = (IAudioProcessor*) i;
	}
	else
	{
		instance = Steinberg::Vst::VST3Controller::createInstance(0);
	}

	if (instance)
	{
		if (instance->queryInterface (iid, obj) == kResultOk)
		{
			instance->release ();
			return kResultOk;
		}
		else
			instance->release ();
	}

	*obj = 0;
	return kNoInterface;
}

void MyVstPluginFactory::initialize()
{
	static auto callOnce = initializeFactory();
}

void MyVstPluginFactory::RegisterPin(
	tinyxml2::XMLElement* pin,
	std::vector<pinInfoSem>* pinlist,
	int32_t plugin_sub_type,
	int nextPinId
)
{
	assert(pin);
	int type_specific_flags = 0;

	if (plugin_sub_type != gmpi::MP_SUB_TYPE_AUDIO)
	{
//?		type_specific_flags = IO_UI_COMMUNICATION;
	}

	pinInfoSem pind{};

	//		_RPT1(_CRT_WARN, "%s\n", pins[i]->GetXML() );
	// id
//	int pin_id; // = XStr2Int( pin->Attribute("id") ));
	pind.id = nextPinId;
	pin->QueryIntAttribute("id", &(pind.id));
	// name
	pind.name = FixNullCharPtr(pin->Attribute("name"));

	// datatype
	std::string pin_datatype = FixNullCharPtr(pin->Attribute("datatype"));
	std::string pin_rate = FixNullCharPtr(pin->Attribute("rate"));

	// Datatype.
	int temp;
	if (XmlStringToDatatype(pin_datatype, temp))
	{
		assert(0 <= temp && temp < (int)gmpi::PinDatatype::Blob);
		pind.datatype = (gmpi::PinDatatype)temp;
/*
		if (pind.datatype == DT_CLASS) // e.g. "class:geometry"
		{
			if (pin_datatype.size() > 6)
				pind.classname = pin_datatype.substr(6);
		}
*/
	}
	else
	{
		//std::wostringstream oss;
		//oss << L"err. module XML file (" << Filename() << L"): parameter id " << pin_id << L" unknown datatype '" << Utf8ToWstring(pin_datatype) << L"'. Valid [float, int ,string, blob, midi ,bool ,enum ,double]";

#if defined( SE_EDIT_SUPPORT )
		Messagebox(oss);
#endif
	}

	// default
	pind.default_value = FixNullCharPtr(pin->Attribute("default"));

	if (pin_rate.compare("audio") == 0)
	{
		if (!pind.default_value.empty())
		{
			// multiply default by 10 (to Volts). DoubleToString() removes trilaing zeros.
			char* temp;	// convert string to SAMPLE
			pind.default_value = std::to_string(10.0f * (float)strtod(pind.default_value.c_str(), &temp));
		}

		pind.datatype = gmpi::PinDatatype::Audio;

		if (pin_datatype.compare("float") != 0)
		{
			//std::wostringstream oss;
			//oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" audio-rate not supported.";
			//Messagebox(oss);
		}
	}

	// direction
	const std::string pin_direction = FixNullCharPtr(pin->Attribute("direction"));

	if (pin_direction.compare("out") == 0)
	{
		pind.direction = gmpi::PinDirection::Out;

#if defined( SE_EDIT_SUPPORT )
		if (!pind.default_value.empty())
		{
			SafeMessagebox(0, (L"module data: default not supported on output pin"));
		}
#endif
	}
	else
	{
		pind.direction = gmpi::PinDirection::In;
	}

	// flags
	pind.flags = type_specific_flags;
#if 0 // TODO
	SetPinFlag("private", IO_HIDE_PIN, pin, pind.flags);
	SetPinFlag("autoRename", IO_RENAME, pin, pind.flags);
	SetPinFlag("isFilename", IO_FILENAME, pin, pind.flags);
	SetPinFlag("linearInput", IO_LINEAR_INPUT, pin, pind.flags);
	SetPinFlag("ignorePatchChange", IO_IGNORE_PATCH_CHANGE, pin, pind.flags);
	SetPinFlag("autoDuplicate", IO_AUTODUPLICATE, pin, pind.flags);
	SetPinFlag("isMinimised", IO_MINIMISED, pin, pind.flags);
	SetPinFlag("isPolyphonic", IO_PAR_POLYPHONIC, pin, pind.flags);
	SetPinFlag("autoConfigureParameter", IO_AUTOCONFIGURE_PARAMETER, pin, pind.flags);
	SetPinFlag("noAutomation", IO_PARAMETER_SCREEN_ONLY, pin, pind.flags);
	SetPinFlag("redrawOnChange", IO_REDRAW_ON_CHANGE, pin, pind.flags);
	SetPinFlag("isContainerIoPlug", IO_CONTAINER_PLUG, pin, pind.flags);
	SetPinFlag("isAdderInputPin", IO_ADDER, pin, pind.flags);

	std::wstring pin_automation = Utf8ToWstring(pin->Attribute("automation"));
#endif

#if defined( SE_EDIT_SUPPORT )
	if (pin->Attribute("isParameter") != 0)
	{
		SafeMessagebox(0, (L"'isParameter' not allowed in pin XML"));
	}
	if (!pin_automation.empty())
	{
		SafeMessagebox(0, (L"'automation' not allowed in pin XML"));
	}
#endif

	// parameter ID. Defaults to ssame as pin ID, but can be overridden.
	// Pins can be driven from patch-store.
	int parameterId = -1;
	pin->QueryIntAttribute("parameterId", &parameterId);

	//if (parameterId != -1)
	//{
	//	pind.flags |= (IO_PATCH_STORE | IO_HIDE_PIN);
	//}

	// host-connect
	pind.hostConnect = FixNullCharPtr(pin->Attribute("hostConnect"));

	// parameterField.
#if 0
	auto parameterFieldId = FT_VALUE;
	if (!pind.hostConnect.empty() || parameterId != -1)
	{
#if defined( SE_TARGET_PLUGIN)
		// In exported VST3s, just using int in XML. Faster, more compact.
		pin->QueryIntAttribute("parameterField", &parameterFieldId);
#else

		// see matching enum ParameterFieldType
		string parameterField(FixNullCharPtr(pin->Attribute("parameterField")));
		if (!XmlStringToParameterField(parameterField, parameterFieldId))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" unknown Parameter Field ID.";

#if defined( SE_EDIT_SUPPORT )
			Messagebox(oss);
#endif
		}
#endif
	}

	if (!pind.hostConnect.empty())
	{
		pind.flags |= IO_HOST_CONTROL | IO_HIDE_PIN;
		HostControls hostControlId = (HostControls)StringToHostControl(pind.hostConnect.c_str());
		if (hostControlId == HC_NONE)
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML: '" << pind.hostConnect << L"' unknown HOST CONTROL.";
			Messagebox(oss);
		}

		if (parameterFieldId == FT_VALUE)
		{
			int expectedDatatype = GetHostControlDatatype(hostControlId);
			if (expectedDatatype == DT_ENUM)
			{
				expectedDatatype = DT_INT;
			}

			if (parameterFieldId == FT_VALUE && expectedDatatype != -1 && expectedDatatype != pind.datatype)
			{
				std::wostringstream oss;
				oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" hostConnect wrong datatype. Expected: " << expectedDatatype;
				Messagebox(oss);
			}
		}

		if (pind.direction != DR_IN && (hostControlId < HC_USER_SHARED_PARAMETER_INT0 || hostControlId > HC_USER_SHARED_PARAMETER_INT4))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" hostConnect pin wrong direction. Expected: direction=\"in\"";
			Messagebox(oss);
		}
	}
#endif
	// meta data
	pind.meta_data = FixNullCharPtr(pin->Attribute("metadata"));
	
	// notes
//	pind.notes = Utf8ToWstring(pin->Attribute("notes"));
#if 0
	if (pinlist->find(pin_id) != pinlist->end())
	{
		//std::wostringstream oss;
		//oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" used twice.";
		//Messagebox(oss);
	}
	else
#endif
	{
		//InterfaceObject* iob = new InterfaceObject(pin_id, pind);
		//// iob->setAutomation(controllerType);
		//iob->setParameterId(parameterId);
		//iob->setParameterFieldId(parameterFieldId);

		pind.parameterId = parameterId;

		// constraints
#if 0
		// Can't have isParameter on output Gui Pin.
		if (iob->GetDirection() == DR_OUT && iob->isUiPlug(0) && iob->isParameterPlug(0))
		{
			if (
				m_unique_id != L"Slider" && // exception for old crap.
				m_unique_id != L"List Entry" &&
				m_unique_id != L"Text Entry" &&
				m_group_name != L"Debug"
				)
			{
				std::wostringstream oss;
				oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" Can't have isParameter on output Gui Pin";
				Messagebox(oss);
			}
		}

		// Patch Store pins must be hidden.
		if (iob->isParameterPlug(0) && !iob->DisableIfNotConnected(0))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" Patch-store input pins must have private=true.";
			Messagebox(oss);
		}

		// DSP parameters only support FT_VALUE
		if (iob->getParameterFieldId(0) != FT_VALUE && !iob->isUiPlug(0) && iob->isParameterPlug(0))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" parameterField not supported on Audio (DSP) pins.";
			Messagebox(oss);
		}

		// parameter used by pin but not declared in "Parameters" section.
		if (iob->isParameterPlug(0) && m_parameters.find(iob->getParameterId(0)) == m_parameters.end())
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" parameter: " << iob->getParameterId(0) << L" not defined";
			Messagebox(oss);
		}

		// parameter used on autoduplicate pin.
		if (iob->isParameterPlug(0) && iob->autoDuplicate(0))
		{
			std::wostringstream oss;
			oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L"  - can't have both 'autoDuplicate' and 'parameterId'";
			Messagebox(oss);
		}
#endif
		//std::pair< module_info_pins_t::iterator, bool > res = pinlist->insert(std::pair<int, InterfaceObject*>(pin_id, iob));
		//assert(res.second && "pin already registered");

		pinlist->push_back(pind);
	}
}

void MyVstPluginFactory::RegisterXml(const char* xml)
{
	tinyxml2::XMLDocument doc;
	doc.Parse(xml);

	if (doc.Error())
	{
		//std::wostringstream oss;
		//oss << L"Module XML Error: [" << full_path << L"]" << doc.ErrorDesc() << L"." << doc.Value();
		//SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
		return;
	}

	//	RegisterPluginsXml(&doc, pluginPath);
	auto pluginList = doc.FirstChildElement("PluginList");
	if (!pluginList)
		return;

	for (auto pluginE = pluginList->FirstChildElement("Plugin"); pluginE; pluginE = pluginE->NextSiblingElement())
	{
		const std::string id(pluginE->Attribute("id"));
		if (id.empty())
		{
			continue; // error
		}

		plugins.push_back({});

		auto& info = plugins.back();
		info.id = id;

		info.name = pluginE->Attribute("name");
		if (info.name.empty())
		{
			info.name = info.id;
		}

		if (auto s = pluginE->Attribute("vendor"); s)
			vendorName_ = s;
		else
			vendorName_ = "GMPI";

		auto parameters = pluginE->FirstChildElement("Parameters");

		int scanTypes[] = { gmpi::MP_SUB_TYPE_AUDIO, gmpi::MP_SUB_TYPE_GUI, gmpi::MP_SUB_TYPE_CONTROLLER };
		std::vector<pinInfoSem>* pinList = {};
		for (auto sub_type : scanTypes)
		{
			const char* sub_type_name = {};

			switch (sub_type)
			{
			case gmpi::MP_SUB_TYPE_AUDIO:
				sub_type_name = "Audio";
				pinList = &info.dspPins;
				break;

			case gmpi::MP_SUB_TYPE_GUI:
				sub_type_name = "GUI";
				pinList = &info.guiPins;
				break;

			case gmpi::MP_SUB_TYPE_CONTROLLER:
				sub_type_name = "Controller";
				pinList = nullptr;
				break;
			}

			auto classE = pluginE->FirstChildElement(sub_type_name);
			if (!classE)
				continue;

			if (sub_type == gmpi::MP_SUB_TYPE_AUDIO)
			{
				// plugin_class->ToElement()->QueryIntAttribute("latency", &latency);
			}

			if (pinList)
			{
				int nextPinId = 0;
				for (auto pinE = classE->FirstChildElement("Pin"); pinE; pinE = pinE->NextSiblingElement("Pin"))
				{
					RegisterPin(pinE, pinList, sub_type, nextPinId);
					++nextPinId;
				}
			}
		}
	}
}

bool MyVstPluginFactory::initializeFactory()
{
	const auto semFolderSearch = BundleInfo::instance()->getSemFolder() + L"/*.gmpi";

	platform_string pluginPath;

	FileFinder it(semFolderSearch.c_str());
	for (; !it.done(); ++it)
	{
		if (!(*it).isFolder)
		{
			pluginPath = (*it).fullPath;
			break;
		}
	}

	if (pluginPath.empty())
	{
		return true;
	}

	gmpi_dynamic_linking::DLL_HANDLE hinstLib;
	gmpi_dynamic_linking::MP_DllLoad(&hinstLib, pluginPath.c_str());

	if (!hinstLib)
	{
		return true;
	}

#if _WIN32
	// Use XML data to get list of plugins
	auto hInst = (HINSTANCE)hinstLib;
	HRSRC hRsrc = ::FindResource(hInst,
		MAKEINTRESOURCE(1), // ID
		L"GMPXML");			// type GMPI XML
#else
#error implement this for mac
#endif

	if (hRsrc)
	{
		const BYTE* lpRsrc = (BYTE*)LoadResource(hInst, hRsrc);

		if (lpRsrc)
		{
			const BYTE* locked_mem = (BYTE*)LockResource((HANDLE)lpRsrc);
			const std::string xmlFile((char*)locked_mem);

			// cleanup
			UnlockResource((HANDLE)lpRsrc);
			FreeResource((HANDLE)lpRsrc);
			gmpi_dynamic_linking::MP_DllUnload(hinstLib);

			RegisterXml(xmlFile.c_str());

			return true;
		}
	}

	// Shell plugins
	// GMPI & sem V3 export function
	gmpi::MP_DllEntry dll_entry_point;
	const char* gmpi_dll_entrypoint_name = "MP_GetFactory";
	auto r = gmpi_dynamic_linking::MP_DllSymbol(hinstLib, gmpi_dll_entrypoint_name, (void**)&dll_entry_point);

	if (r != gmpi::MP_OK) // GMPI/SDK3 Plugin
	{
		gmpi_dynamic_linking::MP_DllUnload(hinstLib);
		return false;
	}

	{ // restrict scope of 'vst_factory' and 'gmpi_factory' so smart pointers RIAA before dll is unloaded

		// Instansiate factory and query sub-plugins.
		gmpi_sdk::mp_shared_ptr<gmpi::IMpShellFactory> vst_factory;
		gmpi_sdk::mp_shared_ptr<gmpi::api::IPluginFactory> gmpi_factory;
		{
			gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> com_object;
			r = dll_entry_point(com_object.asIMpUnknownPtr());

			r = com_object->queryInterface(gmpi::MP_IID_SHELLFACTORY, vst_factory.asIMpUnknownPtr());
			r = com_object->queryInterface((const gmpi::MpGuid&)gmpi::api::IPluginFactory::guid, gmpi_factory.asIMpUnknownPtr());
		}

		if (!vst_factory && !gmpi_factory)
		{
			//std::wostringstream oss;
			//oss << L"Module missing XML resource, and has no factory\n" << full_path;
			//SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);

			gmpi_dynamic_linking::MP_DllUnload(hinstLib);
			return false;
		}

		// if we found VST shell but we're only scanning SEMs, exit.
//		if ((vst_factory && !scanVstsOnly) || (gmpi_factory && scanVstsOnly))
		if (!vst_factory && !gmpi_factory)
		{
			vst_factory = nullptr;
			gmpi_factory = nullptr;
			gmpi_dynamic_linking::MP_DllUnload(hinstLib);
			return false;
		}

		int index = 0;
		while (true)
		{
//			MpString s;
			gmpi::MpString s;
			if (gmpi_factory)
			{
				// have to cast GMPI 2 types to GMPI 1 types
				r = (int32_t)gmpi_factory->getPluginInformation(index++, /*(gmpi::api::IString*)&*/&s); // FULL XML
			}
			//else
			//{
			//	r = vst_factory->getPluginIdentification(index++, s.getUnknown()); // Summary XML
			//}

			if (r != gmpi::MP_OK)
				break;

			RegisterXml(s.c_str());
			//TiXmlDocument doc2;
			//doc2.Parse(s.c_str());

			//if (doc2.Error())
			//{
			//	std::wostringstream oss;
			//	oss << L"Module XML Error: [SynthEdit.exe]" << doc2.ErrorDesc() << L"." << doc2.Value();
			//	SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
			//	break;
			//}

			//ModuleFactory()->RegisterExternalPluginsXml(&doc2, full_path, group_name, scanVstsOnly);
		}
	}

#if 0 // TODO
	auto factoryXml = BundleInfo::instance()->getResource( "factory.se.xml" );

	TiXmlDocument doc;
	doc.Parse( factoryXml.c_str() );

	if ( doc.Error() )
	{
		TiXmlNode* e = doc.FirstChildElement();
		while( e )
		{
			_RPT1(_CRT_WARN, "%s\n", e->Value() );
			TiXmlElement* pElem = e->FirstChildElement("From");
			if( pElem )
			{
				const char* from = pElem->Value();
				from = from; // to shut compiler up.
			}
			e = e->LastChild();
		}
		assert(false);
	}
	else
	{
		TiXmlHandle hDoc(&doc);
		TiXmlElement* pElem;

		// block: Vendor
		pElem=hDoc.FirstChildElement().Element();

		// should always have a valid root but handle gracefully if it does
		if (pElem)
		{
			// First element must be the vendor.
			assert( strcmp(pElem->Value(), "Factory" ) == 0 );

			TiXmlElement* vendor = pElem->FirstChild( "Vendor" )->ToElement();

			vendorName_ = vendor->Attribute("Name");
			vendorUrl_.assign(vendor->Attribute("Url"));
			vendorEmail_.assign(vendor->Attribute("Email"));
			vendorEmail_ = "mailto:" + vendorEmail_; // Steinberg convention to express it like a web link.

			TiXmlNode* plugins = pElem->FirstChild( "Plugins" );
			assert( plugins ); // Got to have one.
			TiXmlNode* pluginNode = plugins->FirstChild( "Plugin" );
			assert( pluginNode ); // Got to have one.

			TiXmlElement* plugin = pluginNode->ToElement();

			pluginInfo_.name_ = plugin->Attribute("Name");
			pluginInfo_.version_ = plugin->Attribute("Version");
			pluginInfo_.processorId.fromRegistryString( plugin->Attribute("ProcessorId") );
			pluginInfo_.controllerId.fromRegistryString( plugin->Attribute("ControllerId") );
				
			backwardCompatible4charId = 0;
			plugin->QueryIntAttribute("PluginID", &backwardCompatible4charId);

			pluginInfo_.subCategories_ = plugin->Attribute("subCategories");
			pluginInfo_.ElatencyCompensation = 0; // none.
			plugin->QueryIntAttribute("latencyCompensation", &pluginInfo_.ElatencyCompensation);

			plugin->QueryBoolAttribute("outputsAsStereoPairs", &pluginInfo_.outputsAsStereoPairs);
			plugin->QueryBoolAttribute("emulateIgnorePC", &pluginInfo_.emulateIgnorePC);
			
			pluginInfo_.outputNames = plugin->Attribute("outputNames");
   
            BundleInfo::instance()->setPluginId(backwardCompatible4charId);
		}
	}

	// VST3 preset folder
	{
		// https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Preset+Locations.html
		auto vst3PresetFolder = BundleInfo::instance()->getUserDocumentFolder();

#if defined( _WIN32 )
		// Correct folder (user presets). [MYDOCUMENTS]/VST3 Presets/$COMPANY/$PLUGIN-NAME/
		vst3PresetFolder += L"VST3 Presets\\";

#else
		// Note: !!! This folder may not exist, which will result in presets being saved in 'Documents' (and never scanned by preset browser)
// might be prefereable to check if this directory exists, and if not fallback to 'Documents' on mac
		vst3PresetFolder += L"/Library/Audio/Presets/";
#endif

		// Add Company name.
		BundleInfo::instance()->presetFolder =
		vst3PresetFolder +
		Utf8ToWstring(getVendorName()) + L"/" +       // Vendor
		Utf8ToWstring(getProductName()) + L"/";       // Product.
	}
#else
	vendorName_ = "SynthEdit";
	pluginInfo_.name_ = "SEM Wrapper";
	pluginInfo_.processorId.fromRegistryString("{8A389500-D21D-45B6-9FA7-F61DEFA68328}");
	pluginInfo_.controllerId.fromRegistryString("{D3047E63-2F3F-43A8-96AC-68D068F56106}");
#endif
	return true;
}

//------------------------------------------------------------------------
//IMPLEMENT_REFCOUNT (MyVstPluginFactory)

//------------------------------------------------------------------------
tresult PLUGIN_API MyVstPluginFactory::queryInterface (FIDString iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, IPluginFactory::iid, IPluginFactory)
	QUERY_INTERFACE (iid, obj, IPluginFactory2::iid, IPluginFactory2)
	QUERY_INTERFACE (iid, obj, IPluginFactory3::iid, IPluginFactory3)
	QUERY_INTERFACE (iid, obj, FUnknown::iid, FUnknown)
	*obj = 0;
	return kNoInterface;
}

#if 0 // ?? !defined( _WIN32 ) // Mac Only.
std::wstring MyVstPluginFactory::getSemFolder()
{
    std::string result;

    CFBundleRef br = CreatePluginBundleRef();
    
	if ( br ) // getBundleRef ())
	{
        CFURLRef url2 = CFBundleCopyBuiltInPlugInsURL (br);
        char filePath2[PATH_MAX] = "";
        if (url2)
        {
            if (CFURLGetFileSystemRepresentation (url2, true, (UInt8*)filePath2, PATH_MAX))
            {
                result = filePath2;
            }
        }
        ReleasePluginBundleRef(br);
    }
    
    return Utf8ToWstring(result.c_str());
}            
#endif


bool MyVstPluginFactory::GetOutputsAsStereoPairs()
{
	return pluginInfo_.outputsAsStereoPairs;
}

std::string MyVstPluginFactory::getVendorName()
{
	return vendorName_;
}

std::wstring MyVstPluginFactory::GetOutputsName(int index)
{
	it_enum_list it( Utf8ToWstring( pluginInfo_.outputNames ));

	it.FindIndex(index);
	if( !it.IsDone() )
	{
		return ( *it )->text;
	}

	return L"AudioOutput";
}


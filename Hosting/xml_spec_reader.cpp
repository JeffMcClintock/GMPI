#include <map>
#include <unordered_map>
#include <array>

#include "xml_spec_reader.h"
#include "Common.h"
#include "tinyXml2/tinyxml2.h"
#include "GmpiSdkCommon.h"
#include "BundleInfo.h"

#if 0
#include "it_enum_list.h"
#include "FileFinder.h"
#include "FileFinder.h"
#include "GmpiApiAudio.h"
#endif

extern "C"
gmpi::ReturnCode MP_GetFactory( void** returnInterface );

namespace gmpi
{
namespace hosting
{

static const std::unordered_map<std::string, gmpi::Field> parameterFieldInfo =
{
	  {"Grab",				gmpi::Field::Grab}
	, {"Value",				gmpi::Field::Value}
	, {"Private",			gmpi::Field::Private}
	, {"Default",			gmpi::Field::Default}
	, {"EnumList",			gmpi::Field::EnumList}
	, {"ShortName",			gmpi::Field::ShortName}
	, {"MenuItems",			gmpi::Field::MenuItems}
	, {"Normalized",		gmpi::Field::Normalized}
	, {"Automation",		gmpi::Field::Automation}
	, {"RangeMinimum",		gmpi::Field::RangeLo}
	, {"RangeMaximum",		gmpi::Field::RangeHi}
	, {"MenuSelection",		gmpi::Field::MenuSelection}
	, {"FileExtension",		gmpi::Field::FileExtension}
	, {"Automation Sysex",	gmpi::Field::AutomationSysex}
	, {"IgnoreProgramChange",gmpi::Field::IgnoreProgramChange}
};

static const std::unordered_map<std::string, gmpi::PinDatatype> pinDatatypeInfo =
{
	{"float",		gmpi::PinDatatype::Float32},
	{"int",			gmpi::PinDatatype::Int32},
	{"string",		gmpi::PinDatatype::String},
	{"blob",		gmpi::PinDatatype::Blob},
	{"midi",		gmpi::PinDatatype::Midi},
	{"bool",		gmpi::PinDatatype::Bool},
	{"enum",		gmpi::PinDatatype::Enum},
	{"double",		gmpi::PinDatatype::Float64},
	{"audio",		gmpi::PinDatatype::Audio},
};

static const std::unordered_map<std::string, HostControls> hostControlsInfo =
{
	{"None",					HostControls::None},
	{"TimeBpm",					HostControls::TimeBpm},
	{"TimeBarStart",			HostControls::TimeBarStart},
	{"TimeNumerator",			HostControls::TimeNumerator},
	{"TimeDenominator",			HostControls::TimeDenominator},
	{"TimeTransportPlaying",	HostControls::TimeTransportPlaying},
	{"TimeQuarterNotePosition",	HostControls::TimeQuarterNotePosition},

	{"ProcessBypass",			HostControls::ProcessBypass },
	{"ProcessRendermode",		HostControls::ProcessRendermode},
};

static std::array< gmpi::PinDatatype, 8 > hostControlDatatypes = {
	  gmpi::PinDatatype::Float32 // TimeBpm
	, gmpi::PinDatatype::Float32 // TimeBarStart
	, gmpi::PinDatatype::Int32   // TimeNumerator
	, gmpi::PinDatatype::Int32   // TimeDenominator
	, gmpi::PinDatatype::Bool    // TimeTransportPlaying
	, gmpi::PinDatatype::Float32 // TimeQuarterNotePosition

	, gmpi::PinDatatype::Int32   // ProcessRendermode
	, gmpi::PinDatatype::Bool    // ProcessBypass
};

static std::array< const char*, 8 > hostControlNiceNames = {
	  "BPM"
	, "Bar Start"
	, "Numerator"
	, "Denominator"
	, "Transport Playing"
	, "Quarter Note Position"
	, "Bypass"
	, "Render Mode"
};

template <typename E>
std::optional<E> lookup(std::string s, const std::unordered_map<std::string, E>& map)
{
	if (auto it = map.find(s); it != map.end())
	{
		return it->second;
	}
	return {};
}

std::string QueryStringAttribute(tinyxml2::XMLElement* e, const char* attribute_name)
{
	if (const auto at = e->Attribute(attribute_name); at)
	{
		return at;
	}
	return {};
}

std::optional<HostControls> intToHostControlSafe(int hc)
{
	if (hc >= (int)HostControls::TimeBpm && hc <= (int)HostControls::ProcessBypass)
	{
		return (HostControls)hc;
	}
	return {};
}

std::string getHostControlNiceName(HostControls hc)
{
	if (hc == HostControls::None)
		return "<none>";

	const int index = (int)hc;
	assert(index >= 0 && index < (int)hostControlNiceNames.size());

	return hostControlNiceNames[index];
}

gmpi::PinDatatype getHostControlDatatype(HostControls hc)
{
	if (hc == HostControls::None)
		return gmpi::PinDatatype::Float32;

	const int index = (int)hc;
	assert(index >= 0 && index < (int)hostControlDatatypes.size());
	return hostControlDatatypes[index];
}

void RegisterPin(
	pluginInfo& info,
	tinyxml2::XMLElement* pin,
	std::vector<pinInfo>* pinlist,
	gmpi::api::PluginSubtype plugin_sub_type,
	int nextPinId
)
{
	assert(pin);
	int type_specific_flags = 0;

	if (plugin_sub_type != gmpi::api::PluginSubtype::Audio)
	{
		//?		type_specific_flags = IO_UI_COMMUNICATION;
	}

	pinInfo pind{};

	pind.id = nextPinId;
	pin->QueryIntAttribute("id", &(pind.id));

	// name
	pind.name = QueryStringAttribute(pin, "name");

	// parameter and host-connect pins. doing this first to determine expected pin datatype.
	pind.parameterFieldType = gmpi::Field::Value;
	int expectedPinDatatype = -1;
	{
		if (const auto parameterIdAt = pin->Attribute("parameterId"); parameterIdAt)
		{
			sscanf(parameterIdAt, "%d", &pind.parameterId);
		}

		// host-connect
		pind.hostConnect = lookup<HostControls>(QueryStringAttribute(pin, "hostConnect"), hostControlsInfo).value_or(HostControls::None);

		// parameterField.
		if (pind.hostConnect != HostControls::None || pind.parameterId != -1)
		{
			if (const auto parameterField = pin->Attribute("parameterField"); parameterField)
			{
				// field id can be stored as int (plugin XML), or as enum text (sems XML)
				if (isdigit(*parameterField))
				{
					sscanf(parameterField, "%d", (int*)&pind.parameterFieldType);
				}
				else
				{
					pind.parameterFieldType = lookup(parameterField, parameterFieldInfo).value_or(gmpi::Field::Value);
					//if (auto field = lookup(parameterField, parameterFieldInfo) ; field)
					//{
					//	pind.parameterFieldType = field.value();
					//}
					//else
					{
						//std::wostringstream oss;
						//oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" unknown Parameter Field ID.";
						//Messagebox(oss);
					}
				}
			}

			if (pind.parameterId != -1) // parameter pin
			{
				//				pind.flags |= (IO_PATCH_STORE | IO_HIDE_PIN);

				switch (pind.parameterFieldType)
				{
				case gmpi::Field::EnumList:
				case gmpi::Field::FileExtension:
				case gmpi::Field::MenuItems:
				case gmpi::Field::ShortName:
				case gmpi::Field::LongName:
				case gmpi::Field::AutomationSysex:
					expectedPinDatatype = (int)gmpi::PinDatatype::String;
					break;

				case gmpi::Field::Grab:
				case gmpi::Field::Private:
				case gmpi::Field::Stateful:
				case gmpi::Field::IgnoreProgramChange:
					expectedPinDatatype = (int)gmpi::PinDatatype::Bool;
					break;

				case gmpi::Field::MenuSelection:
				case gmpi::Field::Automation:
					expectedPinDatatype = (int)gmpi::PinDatatype::Int32;
					break;

				case gmpi::Field::Normalized:
					expectedPinDatatype = (int)gmpi::PinDatatype::Float32;
					break;

				case gmpi::Field::RangeLo:
				case gmpi::Field::RangeHi:
				case gmpi::Field::Value:
					// uses parameter datatype.
					for (auto& param : info.parameters)
					{
						if (param.id == pind.parameterId)
						{
							expectedPinDatatype = (int)param.datatype;
							break;
						}
					}
					break;

				default:
					assert(false); // Missing enum!!
				};
			}
			else // host-connect pin
			{
				//				pind.flags |= IO_HOST_CONTROL | IO_HIDE_PIN;
				pind.parameterId = -2 - (int)pind.hostConnect; // host controls are indicated with negative parameter ids

				if (pind.hostConnect == HostControls::None)
				{
					//std::wostringstream oss;
					//oss << L"ERROR. module XML: '" << pind.hostConnect << L"' unknown HOST CONTROL.";
					//Messagebox(oss);
				}
				//				if (pind.direction != DR_IN && (hostControlId < HC_USER_SHARED_PARAMETER_INT0 || hostControlId > HC_USER_SHARED_PARAMETER_INT4))
				{
					//std::wostringstream oss;
					//oss << L"ERROR. module XML file (" << Filename() << L"): pin id " << pin_id << L" hostConnect pin wrong direction. Expected: direction=\"in\"";
					//Messagebox(oss);
				}

				assert(pind.hostConnect != HostControls::None);

				expectedPinDatatype = (int) hostControlDatatypes[(int) pind.hostConnect];
				assert(expectedPinDatatype != (int)gmpi::PinDatatype::Enum); // only Int32 allowed for enums.
				//{
				//	expectedPinDatatype = (int)gmpi::PinDatatype::Int32;
				//}
			}
		}
	}

	// default
	pind.default_value = QueryStringAttribute(pin, "default");

	// Datatype.
	if (const auto dt = pin->Attribute("datatype"); dt)
	{
		const std::string pin_datatype(dt);

		if (auto dt = lookup(pin_datatype, pinDatatypeInfo); dt)
		{
			pind.datatype = dt.value();

			if (gmpi::PinDatatype::Float32 == pind.datatype)
			{
				const auto rate = pin->Attribute("rate");
				if (rate && strcmp(rate, "audio") == 0)
				{
					pind.datatype = gmpi::PinDatatype::Audio;

					// multiply default by 10 (to Volts).
					char* temp;
					pind.default_value = std::to_string(10.0f * (float)strtod(pind.default_value.c_str(), &temp));
				}
			}
		}
	}
	else
	{
		// if not explicitly specified, take datatype from parameter or host-control.
		if (expectedPinDatatype != -1)
		{
			pind.datatype = (gmpi::PinDatatype)expectedPinDatatype;
		}
		else
		{
			// blank dataype defaults to DT_FSAMPLE, else it's an error.
			if (dt)
			{
				assert(false);
				//std::wostringstream oss;
				//oss << L"err. module XML file (" << Filename() << L"): pin " << pin_id << L": unknown datatype. Valid [float, int ,string, blob, midi ,bool ,enum ,double]";

				//Messagebox(oss);
			}
		}
	}

	// direction
	const std::string pin_direction = QueryStringAttribute(pin, "direction");

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

	// meta data
	pind.meta_data = QueryStringAttribute(pin, "metadata");

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

void readpluginXml(const char* xml, std::vector<pluginInfo>& plugins)
{
	plugins.clear();

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
//		info.pluginPath = pluginPath;

		info.name = pluginE->Attribute("name");
		if (info.name.empty())
		{
			info.name = info.id;
		}

		if (auto s = pluginE->Attribute("vendor"); s)
			info.vendorName = s;
		else
			info.vendorName = "GMPI";

		// PARAMETERS
		if (auto parametersE = pluginE->FirstChildElement("Parameters"); parametersE)
		{
			int nextId = 0;
			for (auto paramE = parametersE->FirstChildElement("Parameter"); paramE; paramE = paramE->NextSiblingElement("Parameter"))
			{
				paramInfo param{};

				// I.D.
				paramE->QueryIntAttribute("id", &(param.id));
				// Datatype.
				std::string pin_datatype = QueryStringAttribute(paramE, "datatype");
				// Name.
				param.name = QueryStringAttribute(paramE, "name");

				// File extension, enum list or range.
				// param.meta_data = wrapper::FixNullCharPtr(paramE->Attribute("metadata"));
				const auto metadataPtr = paramE->Attribute("metadata");
				if (metadataPtr)
				{
					// File extension, enum list or range.
					param.enum_list = metadataPtr;
				}
				else
				{
					// check for min/max range.
					const auto minPtr = paramE->Attribute("min");
					const auto maxPtr = paramE->Attribute("max");
					param.maximum = maxPtr ? atof(maxPtr) : 10.0f;
					param.minimum = minPtr ? atof(minPtr) : 0.0f;
				}

				// Default.
				param.default_value = QueryStringAttribute(paramE, "default");

#if 0
				// Automation.
				int controllerType = ControllerType::None;
				if (!XmlStringToController(FixNullCharPtr(paramE->Attribute("automation")), controllerType))
				{
					//					std::wostringstream oss;
					//					oss << L"err. module XML file (" << Filename() << L"): pin id " << param.id << L" unknown automation type.";
					//#if defined( SE_EDIT_SUPPORT )
					//					Messagebox(oss);
					//#endif
					controllerType = controllerType << 24;
				}

				param.automation = controllerType;
#endif
				// Datatype.
				if (auto dt = lookup(pin_datatype, pinDatatypeInfo); dt)
				{
					param.datatype = dt.value();

					if (gmpi::PinDatatype::Enum == param.datatype)
						param.datatype = gmpi::PinDatatype::Int32; // store as int32.
				}
				else
				{
					//					std::wostringstream oss;
					//					oss << L"err. module XML file (" << Filename() << L"): parameter id " << param.id << L" unknown datatype '" << Utf8ToWstring(pin_datatype) << L"'. Valid [float, int ,string, blob, midi ,bool ,enum ,double]";
					//#if defined( SE_EDIT_SUPPORT )
					//					Messagebox(oss);
					//#endif
				}

				paramE->QueryBoolAttribute("private", &param.is_private);
#if 0
				SetPinFlag(("private"), IO_PAR_PRIVATE, paramE, param.flags);
				SetPinFlag(("ignorePatchChange"), IO_IGNORE_PATCH_CHANGE, paramE, param.flags);
				// isFilename don't appear to be used???? !!!!!
				SetPinFlag(("isFilename"), IO_FILENAME, paramE, param.flags);
				SetPinFlag(("isPolyphonic"), IO_PAR_POLYPHONIC, paramE, param.flags);
				SetPinFlag(("isPolyphonicGate"), IO_PAR_POLYPHONIC_GATE, paramE, param.flags);
				// exception. default for 'persistant' is true (set in param constructor).
				std::string persistantFlag = FixNullCharPtr(paramE->Attribute("persistant"));

				if (persistantFlag == "false")
				{
					CLEAR_BITS(param.flags, IO_PARAMETER_PERSISTANT);
				}
#endif

#if 0
				// can't handle poly parameters with 128 patches, bogs down dsp_patch_parameter_base::getQueMessage()
				// !! these are not really parameters anyhow, more like controlers (host genereated)
				if ((param.flags & IO_PAR_POLYPHONIC) && (param.flags & IO_IGNORE_PATCH_CHANGE) == 0)
				{
					std::wostringstream oss;
					oss << L"err. '" << GetName() << L"' module XML file: pin id " << param.id << L" - Polyphonic parameters need ignorePatchChange=true";
					Messagebox(oss);
				}
				if (m_parameters.find(param.id) != m_parameters.end())
				{
#if defined( SE_EDIT_SUPPORT )
					std::wostringstream oss;
					oss << L"err. module XML file: parameter id " << param.id << L" used twice.";
					Messagebox(oss);
#endif
				}
				else
				{
					//bool res = m_parameters.insert(std::pair<int, parameter_description*>(param.id, pind)).second;
					//assert(res && "parameter already registered");
				}
#endif
				info.parameters.push_back(param);

				++nextId;
			}
		}

		gmpi::api::PluginSubtype scanTypes[] = { gmpi::api::PluginSubtype::Audio, gmpi::api::PluginSubtype::Editor, gmpi::api::PluginSubtype::Controller };
		std::vector<pinInfo>* pinList = {};
		for (auto sub_type : scanTypes)
		{
			const char* sub_type_name = {};

			switch (sub_type)
			{
			case gmpi::api::PluginSubtype::Audio:
				sub_type_name = "Audio";
				pinList = &info.dspPins;
				break;

			case gmpi::api::PluginSubtype::Editor:
				sub_type_name = "GUI";
				pinList = &info.guiPins;
				break;

			case gmpi::api::PluginSubtype::Controller:
				sub_type_name = "Controller";
				pinList = nullptr;
				break;
			}

			auto classE = pluginE->FirstChildElement(sub_type_name);
			if (!classE)
				continue;

			if (sub_type == gmpi::api::PluginSubtype::Audio)
			{
				// plugin_class->ToElement()->QueryIntAttribute("latency", &latency);
			}

			if (pinList)
			{
				int nextPinId = 0;
				for (auto pinE = classE->FirstChildElement("Pin"); pinE; pinE = pinE->NextSiblingElement("Pin"))
				{
					RegisterPin(info, pinE, pinList, sub_type, nextPinId);
					++nextPinId;
				}
			}
		}

		// consolidate host-controls
		{
			std::map< HostControls, int> host_controls;

			for (auto& pin : info.dspPins)
			{
				host_controls[pin.hostConnect] = 1;
			}
			for (auto& pin : info.guiPins)
			{
				host_controls[pin.hostConnect] |= 2;
			}

			//int nextId = info.parameters.empty() ? 0 : info.parameters.back().id + 1;
			for (auto [hc, unused] : host_controls)
			{
				if (HostControls::None == hc)
					continue;

				paramInfo param{};
				//param.id = - 1 - (int)hc; // ID becomes negative, derived from HC enum.
				param.hostConnect = hc;
				param.name = hostControlNiceNames[(int) hc];
				param.datatype = hostControlDatatypes[(int)hc];
				param.is_private = true;

				// on VST3 the bypass HC is represented by a standard parameter
				if (HostControls::ProcessBypass == param.hostConnect)
				{
					int bypassParamId = -1;
					for (auto& p : info.parameters)
					{
						bypassParamId = (std::max)(bypassParamId, p.id);
					}

					++bypassParamId;

					// treat it like a standard parameter at the end of the list.
					param.id = bypassParamId;

					// update all pins to use the new parameter id.
					for (auto& pin : info.dspPins)
					{
						if (pin.hostConnect == HostControls::ProcessBypass)
						{
							pin.parameterId = param.id;
						}
					}
					for (auto& pin : info.guiPins)
					{
						if (pin.hostConnect == HostControls::ProcessBypass)
						{
							pin.parameterId = param.id;
						}
					}
				}

				info.parameters.push_back(param);
			}
		}
	}
}
#if 0

typedef gmpi::ReturnCode(*MP_DllEntry)(void**);

bool xml_spec_reader::initializeFactory()
{
	/*platform_*/std::string pluginPath;
#if 0
	// load SEM dynamic.
	const auto semFolderSearch = BundleInfo::instance()->getSemFolder() + L"/*.gmpi";

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

	wrapper::gmpi_dynamic_linking::DLL_HANDLE hinstLib;
	wrapper::gmpi_dynamic_linking::MP_DllLoad(&hinstLib, pluginPath.c_str());
#else
	wrapper::gmpi_dynamic_linking::DLL_HANDLE hinstLib{};
	wrapper::gmpi_dynamic_linking::MP_GetDllHandle(&hinstLib);

#endif
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
			wrapper::gmpi_dynamic_linking::MP_DllUnload(hinstLib);

			RegisterXml(pluginPath, xmlFile.c_str());

			return true;
		}
	}
#else
	// not needed for built-in XML  #error implement this for mac
#endif

	// set some fallbacks
	vendorName_ = "SynthEdit";
	//	pluginInfo_.name_ = "SEM Wrapper";
		// TODO derive from SEM id
	//	pluginInfo_.processorId.fromRegistryString("{8A389500-D21D-45B6-9FA7-F61DEFA68328}");
	//	pluginInfo_.controllerId.fromRegistryString("{D3047E63-2F3F-43A8-96AC-68D068F56106}");

		// Shell plugins
		// GMPI & sem V3 export function
		// ReturnCode MP_GetFactory( void** returnInterface )
		/*
		 MP_DllEntry dll_entry_point;

		const char* gmpi_dll_entrypoint_name = "MP_GetFactory";
		auto fail = gmpi_dynamic_linking::MP_DllSymbol(hinstLib, gmpi_dll_entrypoint_name, (void**)&dll_entry_point);

		if (fail) // GMPI/SDK3 Plugin
		{
			gmpi_dynamic_linking::MP_DllUnload(hinstLib);
			return false;
		}
	*/
	{ // restrict scope of 'vst_factory' and 'gmpi_factory' so smart pointers RIAA before dll is unloaded

		// Instantiate factory and query sub-plugins.
//		gmpi::shared_ptr<gmpi::IMpShellFactory> vst_factory;
		gmpi::shared_ptr<gmpi::api::IPluginFactory> gmpi_factory;
		{
			gmpi::shared_ptr<gmpi::api::IUnknown> com_object;
			//           auto r = dll_entry_point(com_object.asIMpUnknownPtr());
			auto r = MP_GetFactory(com_object.put_void());

			//			r = com_object->queryInterface(gmpi::MP_IID_SHELLFACTORY, vst_factory.asIMpUnknownPtr());
			r = com_object->queryInterface(&gmpi::api::IPluginFactory::guid, gmpi_factory.put_void());
		}

		if (/*!vst_factory &&*/ !gmpi_factory)
		{
			//std::wostringstream oss;
			//oss << L"Module missing XML resource, and has no factory\n" << full_path;
			//SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);

			wrapper::gmpi_dynamic_linking::MP_DllUnload(hinstLib);
			return false;
		}

		int index = 0;
		while (gmpi_factory)
		{
			gmpi::ReturnString s;
			const auto r = gmpi_factory->getPluginInformation(index++, &s); // FULL XML

			if (r != gmpi::ReturnCode::Ok)
				break;

			RegisterXml(pluginPath, s.c_str());
		}
	}

	return true;
}

#if 0 // ?? !defined( _WIN32 ) // Mac Only.
std::wstring xml_spec_reader::getSemFolder()
{
	std::string result;

	CFBundleRef br = CreatePluginBundleRef();

	if (br) // getBundleRef ())
	{
		CFURLRef url2 = CFBundleCopyBuiltInPlugInsURL(br);
		char filePath2[PATH_MAX] = "";
		if (url2)
		{
			if (CFURLGetFileSystemRepresentation(url2, true, (UInt8*)filePath2, PATH_MAX))
			{
				result = filePath2;
			}
		}
		ReleasePluginBundleRef(br);
	}

	return Utf8ToWstring(result.c_str());
}
#endif

bool xml_spec_reader::GetOutputsAsStereoPairs()
{
	return true; // for now.  pluginInfo_.outputsAsStereoPairs;
}

std::string xml_spec_reader::getVendorName()
{
	return vendorName_;
}
#endif
#if 0
uint32 hashString(const std::string& s)
{
	uint32_t hash = 5381;
	{
		for (auto c : s)
		{
			hash = ((hash << 5) + hash) + c; // magic * 33 + c
		}
	}

	return hash;
}

void textIdtoUuid(const std::string& id, bool isController, Steinberg::TUID& ret) // else Processor
{
	// hash the id
	const auto hash = hashString(id);// ^ (isController ? 0xff : 0);

	//union helper_t
	//{
	//	char plain[16] = { "PluginGMPI     " }; // UUID version 4 because 'G' is 0x47
	//	Steinberg::TUID tuid;
	//};

	//helper_t helper;
	memcpy(ret, "PluginGMPI     ", 16);
	ret[11] = isController ? 'C' : 'P';

	memcpy(ret + 12, &hash, sizeof(hash));

	//	helper.plain[6] = 0x40; // UUID version 4

	//	return helper.tuid;
}


int32_t xml_spec_reader::getVst2Id64(int32_t pluginIndex) // generated from hash of GUID. Not compatible w 32-bit VSTs.
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
	unsigned char vst2ID[4]{};
	int i2 = 0;
	for (int i = 0; i < sizeof(info.cid); ++i)
	{
		vst2ID[i2] = vst2ID[i2] + info.cid[i];
		i2 = (i2 + 1) & 0x03;
	}

	for (auto& c : vst2ID)
	{
		c = 0x20 + (std::min)(0x7e, c & 0x7f); // make printable
	}

	return *((int32_t*)vst2ID);
}

/** Create a new class instance. */
tresult xml_spec_reader::createInstance(FIDString cid, FIDString iid, void** obj)
{
	FUID classId = (FUID) * (TUID*)cid;
	FUID interfaceId = (FUID) * (TUID*)iid;

	initialize();

	FUnknown* instance{};

	for (auto& sem : plugins)
	{
		Steinberg::TUID procUUid{};
		Steinberg::TUID ctrlUUid{};
		textIdtoUuid(sem.id, false, procUUid);
		textIdtoUuid(sem.id, true, ctrlUUid);

		if (/*interfaceId == IComponent::iid ||*/ classId == Steinberg::FUID(procUUid))
		{
			auto i = new wrapper::SeProcessor(sem);
			i->setControllerClass(ctrlUUid); // associate with controller.
			instance = (IAudioProcessor*)i;
			break;
		}
		else if (classId == Steinberg::FUID(ctrlUUid))
		{
			instance = static_cast<Steinberg::Vst::IEditController*>(new wrapper::VST3Controller(sem));
			break;
		}
	}

	if (instance)
	{
		if (instance->queryInterface(iid, obj) == kResultOk)
		{
			instance->release();
			return kResultOk;
		}
		else
			instance->release();
	}

	*obj = 0;
	return kNoInterface;
}

void xml_spec_reader::initialize()
{
	static auto callOnce = initializeFactory();
}
#endif
}
}

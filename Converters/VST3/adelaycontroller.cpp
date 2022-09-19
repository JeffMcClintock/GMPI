#include "base/source/updatehandler.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstpresetfile.h"
#include "public.sdk/source/common/memorystream.h"
#include "adelaycontroller.h"
#include "MyVstPluginFactory.h"
#include "adelayprocessor.h"
#include "se_datatypes.h" // kill this
#include "RawConversions.h"
#include "unicode_conversion.h"
/*
#include "midi_defs.h"
#include "conversion.h"
#include "it_enum_list.h"
#include "IPluginGui.h"
#include "HostControls.h"
#include "../Shared/jsoncpp/json/json.h"
#include "modules/shared/FileFinder.h"
#include "UgDatabase.h"
#include "../se_sdk3_hosting/GuiPatchAutomator3.h"
#include "BundleInfo.h"
#include "tinyxml/tinyxml.h" // anoyingly defines DEBUG as blank which messes with vstgui. put last.
#include "../tinyXml2/tinyxml2.h"
#include "VstPreset.h"
#include "Vst2Preset.h"

#ifdef _WIN32
#include "SEVSTGUIEditorWin.h"
#include "../../se_vst3/source/MyVstPluginFactory.h"
#include "pluginterfaces/base/funknown.h"
#include "../shared/unicode_conversion.h"
#else
#include "SEVSTGUIEditorMac.h"
#endif
#include "AuPreset.h"
#include "mfc_emulation.h"
using namespace std;
using namespace tinyxml2;
*/
using namespace JmUnicodeConversions;

void SafeMessagebox(
	void* hWnd,
	const wchar_t* lpText,
	const wchar_t* lpCaption = L"",
	int uType = 0
)
{
	_RPTW1(0, L"%s\n", lpText);
}

MpParameterVst3::MpParameterVst3(Steinberg::Vst::VST3Controller* controller, int strictIndex, int ParameterTag, bool isInverted) :
	MpParameter_native(controller),
	vst3Controller(controller),
	isInverted_(isInverted)
{
	hostTag = ParameterTag;
	hostIndex = strictIndex;
}

void MpParameterVst3::updateProcessor(gmpi::FieldType fieldId, int32_t voice)
{
	switch (fieldId)
	{
	case gmpi::MP_FT_GRAB:
		controller_->ParamGrabbed(this, voice);
		break;

	case gmpi::MP_FT_VALUE:
	case gmpi::MP_FT_NORMALIZED:
		vst3Controller->ParamToProcessorViaHost(this, voice);
		break;
	}
}


namespace Steinberg {
namespace Vst {

VST3Controller::VST3Controller() :
	isInitialised(false)
	, isConnected(false)
{
// using tinxml 1?	TiXmlBase::SetCondenseWhiteSpace(false); // ensure text parameters preserve multiple spaces. e.g. "A     B" (else it collapses to "A B")

	// Scan all presets for preset-browser.
	ScanPresets();
}

tresult PLUGIN_API VST3Controller::connect(IConnectionPoint* other)
{
//	_RPT0(_CRT_WARN, "ADelayController::connect\n");
	auto r = EditController::connect(other);

	isConnected = true;

	// Can only init controllers after both VST controller initialised AND controller is connected to processor.
	// So VST2 wrapper aeffect pointer makes it to Processor.
	if(isConnected && isInitialised)
	initSemControllers();

	return r;
}

tresult PLUGIN_API VST3Controller::notify( IMessage* message )
{
	// WARNING: CALLED FROM PROCESS THREAD (In Ableton Live).
	if( !message )
		return kInvalidArgument;

	if( !strcmp( message->getMessageID(), "BinaryMessage" ) )
	{
		const void* data;
		uint32 size;
		if( message->getAttributes()->getBinary( "MyData", data, size ) == kResultOk )
		{
			message_que_dsp_to_ui.pushString(size, (unsigned char*) data);
			message_que_dsp_to_ui.Send();
			return kResultOk;
		}
	}

	return EditController::notify( message );
}

bool VST3Controller::sendMessageToProcessor(const void* data, int size)
{
	auto message = allocateMessage();
	if (message)
	{
		FReleaser msgReleaser(message);
		message->setMessageID("BinaryMessage");

		message->getAttributes()->setBinary("MyData", data, size);
		sendMessage(message);
	}

	return true;
}

void VST3Controller::ParamGrabbed(MpParameter_native* param, int32_t voice)
{
	auto paramID = param->getNativeTag();

	if(param->isGrabbed() )
		beginEdit(paramID);
	else
		endEdit(paramID);
}

void VST3Controller::ParamToProcessorViaHost(MpParameterVst3* param, int32_t voice)
{
	const auto paramID = param->getNativeTag();

	// Usually parameter will have sent beginEdit() already (if it has mouse-down connected properly, else fake it.
	if (!param->isGrabbed())
		beginEdit(paramID);

	performEdit(paramID, param->convertNormalized(param->getNormalized())); // Send the value to DSP.

	if (!param->isGrabbed())
		endEdit(paramID);
}

void VST3Controller::ResetProcessor()
{
	// Currently called when polyphony etc changes, VST2 wrapper ignores this completely, at least in Live.
//	componentHandler->restartComponent(kLatencyChanged); // or kIoChanged might be less overhead for DAW
}

enum class ElatencyContraintType
{
	Off, Constrained, Full
};

struct pluginInformation
{
	int32_t pluginId = -1;
	std::string manufacturerId;
	int32_t inputCount;
	int32_t outputCount;
	int32_t midiInputCount;
	ElatencyContraintType latencyConstraint;
	std::string pluginName;
	std::string vendorName;
	std::string subCategories;
	bool outputsAsStereoPairs;
	bool emulateIgnorePC = {};
	bool monoUseOk;
	bool vst3Emulate16ChanCcs;
	std::vector<std::string> outputNames;
};

//-----------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::initialize (FUnknown* context)
{
	_RPT0(_CRT_WARN, "ADelayController::initialize\n");

	UpdateHandler::instance();

	tresult result = EditController::initialize (context);
	if (result == kResultTrue)
	{
		// Modules database, needed here to load any controllers. e.g. VST2 child plugins.
		{
//?			ModuleFactory()->RegisterExternalPluginsXmlOnce(nullptr);
		}

		MpController::Initialize();

		//const auto& info = BundleInfo::instance()->getPluginInfo();
		pluginInformation info{};

		const auto supportedChannels = info.midiInputCount ? 16 : 0;
		const auto supportAllCC = info.vst3Emulate16ChanCcs;

		// MIDI CC SUPPORT
		wchar_t ccName[] = L"MIDI CC     ";
		for (int chan = 0; chan < supportedChannels; ++chan)
		{
			for (int cc = 0; cc < numMidiControllers; ++cc)
			{
				if (supportAllCC || cc > 127 || cc == 74) // Channel Presure, Pitch Bend and Brightness
				{
					const ParamID ParameterId = MidiControllersParameterId + chan * numMidiControllers + cc;
					swprintf(ccName + 9, std::size(ccName), L"%3d", cc);

					auto param = makeNativeParameter(ParameterId, false);
					param->datatype_ = DT_FLOAT;
					param->name_ = ccName;
					param->minimum = 0;
					param->maximum = 1;
					param->hostControl_ = -1;
					param->isPolyphonic_ = false;
					param->parameterHandle_ = -1;
					param->moduleHandle_ = -1;
					param->moduleParamId_ = -1;
					param->stateful_ = false;

					auto initialVal(ParseToRaw(param->datatype_, "0"));
					param->rawValues_.push_back(initialVal);				// preset 0/

					parameters_.push_back(std::unique_ptr<MpParameter>(param));
				}
			}
		}
	}

	isInitialised = true;

	// Can only init controllers after both VST controller initialised AND controller is connected to processor.
	// So VST2 wrapper aeffect pointer makes it to Processor.
	if (isConnected && isInitialised)
	initSemControllers();

	StartTimer(timerPeriodMs);

	return kResultTrue;
}

// Parameter updated from DAW.
void VST3Controller::update( FUnknown* changedUnknown, int32 message )
{
	EditController::update( changedUnknown, message );

	if( message == IDependent::kChanged )
	{
	assert(false);
		auto parameter = dynamic_cast<Parameter*>( changedUnknown );
		if( parameter )
		{
			/*
			auto& it = vst3IndexToParameter.find(parameter->getInfo().id);
			if (it != vst3IndexToParameter.end())
			{
				const int voiceId = 0;
				update Guis((*it).second, voiceId);
			}
			*/
		}
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag/*out*/)
{
	if (busIndex == 0)
	{
		const int possibleTag = MidiControllersParameterId + channel * numMidiControllers + midiControllerNumber;
		if (tagToParameter.find(possibleTag) != tagToParameter.end())
		{
			tag = possibleTag;
			return kResultTrue;
		}
	}

	//if (busIndex == 0 && channel < supportedChannels && midiControllerNumber >= 0 && midiControllerNumber < numMidiControllers )
	//{
	//	// MIDI CC 0 - 127 + Bender (128) and Aftertouch (129)
	//	tag = MidiControllersParameterId + channel * numMidiControllers + midiControllerNumber;
	//	return kResultTrue;
	//}

	return kResultFalse;
}

IPlugView* PLUGIN_API VST3Controller::createView (FIDString name)
{
#if 0 // TODO
	if (ConstString (name) == ViewType::kEditor)
	{
		// somewhat inefficient to parse entire JSON file just for GUI size
		// would be nice to pass ownership of document to presenter to save it doing the same all over again.
		Json::Value document_json;
		{
			Json::Reader reader;
			reader.parse(BundleInfo::instance()->getResource("gui.se.json"), document_json);
		}

		auto& gui_json = document_json["gui"];

		int width = gui_json["width"].asInt();
		int height = gui_json["height"].asInt();

#ifdef _WIN32
		// DPI of system. only a GUESS at this point of DPI we will be using. (until we know DAW window handle).
		// But we need a rough idea of plugin window size to allow Cubase to arrange menu bar intelligently. Else we get the blank area at top of plugin.
		HDC hdc = ::GetDC(NULL);
		width = (width * GetDeviceCaps(hdc, LOGPIXELSX)) / 96;
		height = (height * GetDeviceCaps(hdc, LOGPIXELSY)) / 96;
		::ReleaseDC(NULL, hdc);
#endif

		//ViewRect estimatedViewRect;
		//estimatedViewRect.top = estimatedViewRect.left = 0;
		//estimatedViewRect.bottom = height;
		//estimatedViewRect.right = width;

#ifdef _WIN32
		return new SEVSTGUIEditorWin(this, width, height);
#else
		return new SEVSTGUIEditorMac(this, width, height);
#endif
	}
#endif
	return {};
}

// Preset Loaded.
tresult PLUGIN_API VST3Controller::setComponentState (IBStream* state)
{
	int32 bytesRead;
	int32 chunkSize = 0;
	std::string chunk;
	state->read( &chunkSize, sizeof(chunkSize), &bytesRead );
	chunk.resize(chunkSize);
	state->read((void*) chunk.data(), chunkSize, &bytesRead);

	setPresetFromDaw(chunk, false);

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
	QUERY_INTERFACE(iid, obj, INoteExpressionController::iid, INoteExpressionController)
	QUERY_INTERFACE(iid, obj, INoteExpressionPhysicalUIMapping::iid, INoteExpressionPhysicalUIMapping)

	return EditController::queryInterface (iid, obj);
}

tresult VST3Controller::setParamNormalized( ParamID tag, ParamValue value )
{
//	_RPT2(_CRT_WARN, "setParamNormalized(%d, %f)\n", tag, value);

	if (auto p = getDawParameter(tag); p)
	{
		const auto n = p->convertNormalized(value);
		p->MpParameter_base::setParameterRaw(gmpi::MP_FT_NORMALIZED, sizeof(n), &n);
	}

	return kResultTrue;
}

void VST3Controller::OnLatencyChanged()
{
	getComponentHandler()->restartComponent(kLatencyChanged);
	_RPT0(_CRT_WARN, "restartComponent(kLatencyChanged)\n");
}

tresult VST3Controller::getParameterInfo(int32 paramIndex, ParameterInfo& info)
{
	auto p = nativeGetParameterByIndex(paramIndex);

	if (!p)
		return kInvalidArgument;

	info.flags = Steinberg::Vst::ParameterInfo::kCanAutomate;
	info.defaultNormalizedValue = 0.0;

	auto temp = ToUtf16(p->name_);
	
	_tstrncpy(info.shortTitle, temp.c_str(), static_cast<Steinberg::uint32>(std::size(info.shortTitle)));
	_tstrncpy(info.title,      temp.c_str(), static_cast<Steinberg::uint32>(std::size(info.title)));

	info.id = p->getNativeTag();
	info.unitId = 0;
    info.stepCount = 0;
	info.units[0] = 0;

	if ((p->datatype_ == DT_INT || p->datatype_ == DT_INT64) && !p->enumList_.empty())
	{
		info.flags |= Steinberg::Vst::ParameterInfo::kIsList;
		it_enum_list it(p->enumList_);
		info.stepCount = (std::max)(0, it.size() - 1);
//?		info.unitId = 0;
	}

	return kResultOk;
}

tresult PLUGIN_API VST3Controller::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
{
	if (auto p = getDawParameter(tag); p)
	{
		const auto s_wide = p->normalisedToString(p->convertNormalized(valueNormalized));
		const auto s_UTF16 = ToUtf16(s_wide);
		strncpy16(string, s_UTF16.c_str(), 128);

		return kResultOk;
	}

	return kInvalidArgument;
}

std::string VST3Controller::loadNativePreset(std::wstring sourceFilename)
{
#if 0 // TODO
	auto filetype = GetExtension(sourceFilename);

	if (filetype == L"vstpreset")
	{
		return VstPresetUtil::ReadPreset(sourceFilename);
	}

	if (filetype == L"aupreset")
	{
		return AuPresetUtil::ReadPreset(sourceFilename);
	}

	if (filetype == L"fxp")
	{
		auto factory = MyVstPluginFactory::GetInstance();
		const int pluginIndex = 0;
		const auto xml = Vst2PresetUtil::ReadPreset(ToPlatformString(sourceFilename), factory->getVst2Id(pluginIndex), factory->getVst2Id64(pluginIndex));

		if(!xml.empty())
		{
			return xml;
		}

		// Prior to v1.4.518 - 2020-08-04, Preset Browser saves VST3 format instead of VST2 format. Fallback to VST3.
		return VstPresetUtil::ReadPreset(sourceFilename);
	}
#endif
	return {};
}

std::vector< MpController::presetInfo > VST3Controller::scanFactoryPresets()
{
#if 0 // todo
	platform_string factoryPresetsFolder(_T("vst2FactoryPresets/"));
	auto factoryPresetFolder = ToPlatformString(BundleInfo::instance()->getImbeddedFileFolder());
	auto fullpath = combinePathAndFile(factoryPresetFolder, factoryPresetsFolder);

	if (fullpath.empty())
	{
		return {};
	}

	return scanPresetFolder(fullpath, _T("xmlpreset"));
#endif
	return {};
}

void VST3Controller::loadFactoryPreset(int index, bool fromDaw)
{
#if 0 // TODO
	platform_string vst2FactoryPresetsFolder(_T("vst2FactoryPresets/"));
	auto PresetFolder = ToPlatformString(BundleInfo::instance()->getImbeddedFileFolder());
	PresetFolder = combinePathAndFile(PresetFolder, vst2FactoryPresetsFolder);

	auto fullFilePath = PresetFolder + ToPlatformString(presets[index].filename);
	auto filenameUtf8 = ToUtf8String(fullFilePath);
	ImportPresetXml(filenameUtf8.c_str());
#endif
}

void VST3Controller::saveNativePreset(const char* filename, const std::string& presetName, const std::string& xml)
{
#if 0
	const auto filetype = GetExtension(std::string(filename));

	// VST2 preset format.
	if(filetype == "fxp")
	{
		auto factory = MyVstPluginFactory::GetInstance();
		Vst2PresetUtil::WritePreset(ToPlatformString(filename), factory->getVst2Id64(0), presetName, xml);
		return;
	}

	// VST3 preset format.
	{
		// Add Company name.
		auto factory = MyVstPluginFactory::GetInstance();
		auto processorId = factory->getPluginInfo().processorId;

		std::string categoryName; // TODO !!!
		VstPresetUtil::WritePreset(JmUnicodeConversions::Utf8ToWstring(filename), categoryName, factory->getVendorName(), factory->getProductName(), &processorId, xml);
	}
#endif
}

bool VST3Controller::OnTimer()
{
    if (!queueToDsp_.empty())
	{
		sendMessageToProcessor(queueToDsp_.data(), queueToDsp_.size());
		queueToDsp_.clear();
	}

	return MpController::OnTimer();
}

int32 VST3Controller::getNoteExpressionCount(int32 busIndex, int16 channel)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0)
        return 0;

	return 4;
}

tresult VST3Controller::getNoteExpressionInfo (int32 busIndex, int16 channel, int32 noteExpressionIndex, NoteExpressionTypeInfo& info)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0 || noteExpressionIndex > 3)
        return kResultFalse;

	memset (&info, 0, sizeof (NoteExpressionTypeInfo));
 
	// might be better to support two generic types than volume and pan.
	NoteExpressionTypeID typeIds[] =
	{
		kVolumeTypeID,
		kPanTypeID,
		kTuningTypeID,
		kBrightnessTypeID,
	};

	info.typeId = typeIds[noteExpressionIndex];
	info.valueDesc.minimum = 0.0;
	info.valueDesc.maximum = 1.0;
	info.valueDesc.defaultValue = 0.5;
	info.valueDesc.stepCount = 0; // we want continuous (no step)
	info.unitId = -1;

	switch (info.typeId)
	{
	case kVolumeTypeID:
	{
		assert(info.typeId == kVolumeTypeID);

		// set some strings
		USTRING("Volume").copyTo(info.title, 128);
		USTRING("Vol").copyTo(info.shortTitle, 128);
		info.valueDesc.defaultValue = 1.0;
	}
	break;

	case kPanTypeID: // polyphonic pan.
	{
		assert(info.typeId == kPanTypeID);

		// set some strings
		USTRING("Pan").copyTo(info.title, 128);
		USTRING("Pan").copyTo(info.shortTitle, 128);

		info.flags = NoteExpressionTypeInfo::kIsBipolar | NoteExpressionTypeInfo::kIsAbsolute; // event is bipolar (centered)
	}
	break;

	case kTuningTypeID: // polyphonic pitch-bend. ('tuning')
	{
		// set the tuning type
		assert(info.typeId == kTuningTypeID);

		// set some strings
		USTRING("Tuning").copyTo(info.title, 128);
		USTRING("Tun").copyTo(info.shortTitle, 128);
		USTRING("Half Tone").copyTo(info.units, 128);

		info.flags = NoteExpressionTypeInfo::kIsBipolar; // event is bipolar (centered)

		// The range you set here, determins the default range in Cubase Note Expression Inspector.
		// Set it to +- 24 for Roli MPE Controller

		// for Tuning the convert functions are : plain = 240 * (norm - 0.5); norm = plain / 240 + 0.5;
		// we want to support only +/- one octave
		constexpr double kNormTuningOneOctave = 0.5 * 24.0 / 120.0;

		info.valueDesc.minimum = 0.5 - kNormTuningOneOctave;
		info.valueDesc.maximum = 0.5 + kNormTuningOneOctave;
//		info.valueDesc.defaultValue = 0.5; // middle of [0, 1] => no detune (240 * (0.5 - 0.5) = 0)
	}
	break;

	case kBrightnessTypeID:
		USTRING("Brightness").copyTo(info.title, 128);
		USTRING("Brt").copyTo(info.shortTitle, 128);
		info.flags = NoteExpressionTypeInfo::kIsAbsolute;
		break;

	default:
        return kResultFalse;
	}
		
	return kResultTrue;
}

tresult VST3Controller::getNoteExpressionStringByValue (int32 busIndex, int16 channel, NoteExpressionTypeID id, NoteExpressionValue valueNormalized , String128 string)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0)
        return kResultFalse;

	switch (id)
	{
	case kTuningTypeID: // polyphonic pitch-bend.
	{
		// here we have to convert a normalized value to a Tuning string representation
		UString128 wrapper;
		valueNormalized = (240 * valueNormalized) - 120; // compute half Tones
		wrapper.printFloat (valueNormalized, 2);
		wrapper.copyTo (string, 128);
	}
	break;

	case kPanTypeID: // polyphonic pan.
	{
		// here we have to convert a normalized value to a Tuning string representation
		UString128 wrapper;
		valueNormalized = valueNormalized * 2.0 - 1.0;
		wrapper.printFloat (valueNormalized, 2);
		wrapper.copyTo (string, 128);
	}
	break;

	default:
	{
		// here we have to convert a normalized value to a Tuning string representation
		UString128 wrapper;
		wrapper.printFloat (valueNormalized, 2);
		wrapper.copyTo (string, 128);
	}
	break;
	}

	return kResultOk;
}

tresult VST3Controller::getNoteExpressionValueByString (int32 busIndex, int16 channel, NoteExpressionTypeID id, const TChar* string, NoteExpressionValue& valueNormalized)
{
    // we accept only the first bus and 1 channel
    if (busIndex != 0 || channel != 0)
        return kResultFalse;

 	switch (id) // polyphonic pitch-bend.
	{
	case kTuningTypeID:
	{
	   // here we have to convert a given tuning string (half Tone) to a normalized value
		String wrapper ((TChar*)string);
		ParamValue tmp;
		if (wrapper.scanFloat (tmp))
		{
			valueNormalized = (tmp + 120) / 240;
			return kResultTrue;
		}
	}
	break;

	case kPanTypeID:
	{
		String wrapper ((TChar*)string);
		ParamValue tmp;
		if (wrapper.scanFloat (tmp))
		{
			valueNormalized = (tmp + 1.0) * 0.5;
			return kResultTrue;
		}
	}
	break;

	default:
	{
		String wrapper ((TChar*)string);
		ParamValue tmp;
		if (wrapper.scanFloat (tmp))
		{
			valueNormalized = tmp;
			return kResultTrue;
		}
	}
	break;
	}

	return kResultOk;
}

tresult PLUGIN_API VST3Controller::getPhysicalUIMapping(int32 busIndex, int16 channel,
	PhysicalUIMapList& list)
{
	if (busIndex == 0 && channel == 0)
	{
		for (uint32 i = 0; i < list.count; ++i)
		{
			NoteExpressionTypeID type = kInvalidTypeID;

			switch (list.map[i].physicalUITypeID)
			{
			case kPUIXMovement:
				list.map[i].noteExpressionTypeID = kTuningTypeID;
				break;

			case kPUIYMovement:
				list.map[i].noteExpressionTypeID = kBrightnessTypeID;
				break;

			//case kPUIPressure:
			//	list.map[i].noteExpressionTypeID = ?;
			//	break;

			default:
				list.map[i].noteExpressionTypeID = kInvalidTypeID;
				break;
			}
		}
		return kResultTrue;
	}
	return kResultFalse;
}

}// namespaces
} // namespaces

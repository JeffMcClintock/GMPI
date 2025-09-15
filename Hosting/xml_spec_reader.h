#pragma once
/*
#include "Hosting/xml_spec_reader.h"
*/

#include <string>
#include <vector>
#include <algorithm>
#include <optional>
#include "GmpiApiCommon.h"
#include "GmpiApiEditor.h"
#include "parse_enum.h"

namespace tinyxml2
{
	class XMLElement;
}

namespace gmpi
{
namespace hosting
{

enum class HostControls : int
{
    None = -1,

	TimeBpm,
	TimeQuarterNotePosition,
	TimeTransportPlaying,
	TimeBarStart,
	TimeNumerator,
	TimeDenominator,

	ProcessRendermode,
	ProcessBypass,


#if 0 // ignoring SynthEdit-specific host controls for now.
    PatchCommands,
    MidiChannel,
    ProgramNamesList,

    Program,
    ProgramName,
    VoiceTrigger,
    VoiceGate,
    VoicePitch,

    VoiceVelocityKeyOn,
    VoiceVelocityKeyOff,
    VoiceAftertouch,
    VoiceVirtualVoiceId,

    VoiceActive,
    Unused, // was VoiceReset
    VoiceAllocationMode, // includes mono-note-priority.

    PitchBender,
    HoldPedal,
    ChannelPressure,


    Polyphony,
    PolyphonyVoiceReserve,
    OversamplingRate, // NOT REALLY automation. Just need a mechanism to restart DSP.
    OversamplingFilter,
//    UserSharedParameterInt0,

    VoiceVolume,
    VoicePan,
    VoicePitchBend,
    VoiceVibrato,
    VoiceExpression,
    VoiceBrightness,
    VoiceUserControl0,
    VoiceUserControl1,
    VoiceUserControl2,

    VoicePortamentoEnable,
    SnapModulationDeprecated,
    Portamento,
    GlideStartPitch,
    BenderRange,
    SubPatchCommands,

    //UserSharedParameterInt1,
    //UserSharedParameterInt2,
    //UserSharedParameterInt3,
    //UserSharedParameterInt4,

    PatchCables,
    SilenceOptimisation,

    ProgramCategory,
    ProgramCategoriesList,

    MpeMode,
    ProgramModified, // 'ProgramModified' and 'CanUndo' are synonymous (assuming undo manager enabled)
    CanUndo,
    CanRedo,

    // leave last
    // VoiceTuning, // pseudo host-control. used only for persistence of tuning table in getPersisentHostControl().

    // NumHostControls
#endif
};

std::string getHostControlNiceName(HostControls hc);
gmpi::PinDatatype getHostControlDatatype(HostControls hc);

struct pinInfo
{
	int32_t id;
	std::string name;
	gmpi::PinDirection direction;
	gmpi::PinDatatype datatype;
	std::string default_value;
	int32_t parameterId = -1;
	gmpi::Field parameterFieldType;
	int32_t flags;
	HostControls hostConnect{ HostControls::None };
	std::string meta_data;
};

// todo might be helpful to flag if it's used on UI/Processor to save on pointless updates
struct paramInfo
{
	// parameter can have id or host control, the special VST3 BYPASS parameter can be both.
	int32_t id{ -1 };		// identifier for parameter on the GMPI side.
	int32_t dawTag{ -1 };	// identifier for parameter on the DAW side. (e.g. VST3 param index).

	HostControls hostConnect{ HostControls::None };

	std::string name;
	gmpi::PinDatatype datatype;
	std::string default_value_s;
	//int32_t parameterId;
	//int32_t flags;
	std::string enum_list;
	std::vector<enum_entry> enum_entries; // parsed from enum_list.

	double minimum = 0.0;
	double maximum = 1.0;
//	double default_value = 0.0;
	bool is_private{};
	bool is_stateful{ true };
};

struct pluginInfo
{
	std::string id;
	std::string name;
//	int inputCount = {};
//	int outputCount = {};
	std::vector<pinInfo> dspPins;
	std::vector<pinInfo> guiPins;
	std::vector<paramInfo> parameters;

	//	platform_string pluginPath;
	std::string pluginPath;
	std::string vendorName;
	std::string vendorUrl;
	std::string vendorEmail;
};

void readpluginXml(const char* xml, std::vector<pluginInfo>& plugins);

std::optional<HostControls> intToHostControlSafe(int hc);

inline int countPins(gmpi::hosting::pluginInfo const& plugin, gmpi::PinDirection direction, gmpi::PinDatatype datatype)
{
	return static_cast<int>(std::count_if(
		plugin.dspPins.begin()
		, plugin.dspPins.end()
		, [direction, datatype](const gmpi::hosting::pinInfo& p) -> bool
		{
			return p.direction == direction && p.datatype == datatype;
		}
	));
}

inline auto getPinName(gmpi::hosting::pluginInfo const& plugin, gmpi::PinDirection direction, int index) -> std::string
{
	int i = 0;
	for (auto& p : plugin.dspPins)
	{
		if (p.direction != direction || p.datatype != gmpi::PinDatatype::Audio)
			continue;

		if (i++ == index)
			return p.name;
	}

	return {};
}

} // namespace hosting
} // namespace gmpi

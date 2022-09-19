#ifndef __adelaycontroller__
#define __adelaycontroller__

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include <pluginterfaces/vst/ivstunits.h>
#include <pluginterfaces/vst/ivstnoteexpression.h>
#include <pluginterfaces/vst/ivstphysicalui.h>
#include <map>
#include "interThreadQue.h"
//#include "mp_gui.h"
#include "Controller.h"
#include "StagingMemoryBuffer.h"
#include "MpParameter.h"
#include "conversion.h"


namespace Steinberg {
	namespace Vst {
		class VST3Controller;
	}
}

class MpParameterVst3 : public MpParameter_native
{
	Steinberg::Vst::VST3Controller* vst3Controller = {};

public:
	MpParameterVst3(Steinberg::Vst::VST3Controller* controller, int strictIndex, int ParameterTag, bool isInverted);

	// some hosts can't handle parameter min > max, if so calculate normalize in reverse.
	float convertNormalized(float normalised) const
	{
		return isInverted_ ? 1.0f - normalised : normalised;
	}

	void updateProcessor(gmpi::FieldType fieldId, int32_t voice) override;

	// not required for VST3.
	void upDateImmediateValue() override {}

	bool isInverted_ = false;
};

namespace Steinberg {
namespace Vst {

// Manages plugin parameters.
//-----------------------------------------------------------------------------
class VST3Controller :
	public EditController,
	public IMidiMapping,
	public MpController,
	public IUnitInfo,
	public INoteExpressionController,
	public INoteExpressionPhysicalUIMapping
{
	static const int numMidiControllers = 130; // usual 128 + Bender.
	bool isInitialised;
	bool isConnected;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	std::map<int, MpParameterVst3* > tagToParameter;	// DAW parameter Index to parameter
	std::vector<MpParameterVst3* > vst3Parameters; // flat list.
    // Hold data until timer can put it in VST3 queue mechanism.
	StagingMemoryBuffer queueToDsp_;
	int supportedChannels = 1;

public:
	VST3Controller();
	tresult PLUGIN_API initialize (FUnknown* context) override;
	tresult PLUGIN_API connect(IConnectionPoint* other) override;
	tresult PLUGIN_API notify( IMessage* message ) override;
	virtual IPlugView* PLUGIN_API createView (FIDString name) override;

	virtual tresult PLUGIN_API setComponentState (IBStream* state) override;

	//testing.
	virtual tresult PLUGIN_API setState(IBStream* state) override
	{
		return kResultOk;
	}
	virtual tresult PLUGIN_API getState(IBStream* state) override
	{
		return kResultOk;
	}
	void ParamGrabbed(MpParameter_native* param, int32_t voice = 0) override;
	void ParamToProcessorViaHost(MpParameterVst3* param, int32_t voice = 0);

	MpParameterVst3* getDawParameter(int nativeTag)
	{
		auto it = tagToParameter.find(nativeTag);
		if (it != tagToParameter.end())
		{
			return (*it).second;
		}

		return {};
	}

	virtual tresult PLUGIN_API setParamNormalized( ParamID tag, ParamValue value ) override;

	// IDependent
	virtual void PLUGIN_API update( FUnknown* changedUnknown, int32 message ) override;

	// MIDI Mapping.
	tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag/*out*/) override;

	static FUnknown* createInstance (void*) { return (IEditController*)new VST3Controller (); }

	//-----------------------------
	DELEGATE_REFCOUNT (EditController)
	tresult PLUGIN_API queryInterface (const char* iid, void** obj) override;
	//-----------------------------

    //---from INoteExpressionController
    int32 PLUGIN_API getNoteExpressionCount (int32 busIndex, int16 channel) SMTG_OVERRIDE;
    tresult PLUGIN_API getNoteExpressionInfo (int32 busIndex, int16 channel, int32 noteExpressionIndex, NoteExpressionTypeInfo& info) SMTG_OVERRIDE;
    tresult PLUGIN_API getNoteExpressionStringByValue (int32 busIndex, int16 channel, NoteExpressionTypeID id, NoteExpressionValue valueNormalized , String128 string) SMTG_OVERRIDE;
    tresult PLUGIN_API getNoteExpressionValueByString (int32 busIndex, int16 channel, NoteExpressionTypeID id, const TChar* string, NoteExpressionValue& valueNormalized) SMTG_OVERRIDE;

	/** Fills the list of mapped [physical UI (in) - note expression (out)] for a given bus index
 * and channel. */
	tresult PLUGIN_API getPhysicalUIMapping(int32 busIndex, int16 channel,
		PhysicalUIMapList& list) override;


	// IUnitInfo
	//------------------------------------------------------------------------
	/** Returns the flat count of units. */
	virtual int32 PLUGIN_API getUnitCount() override { return 1; }

	/** Gets UnitInfo for a given index in the flat list of unit. */
	virtual tresult PLUGIN_API getUnitInfo(int32 unitIndex, UnitInfo& info /*out*/) override
	{
		info.id = kRootUnitId;
		info.name[0] = 0;
		info.parentUnitId = kNoParentUnitId;
		info.programListId = 0;
		return kResultOk;
	}

	/** Component intern program structure. */
	/** Gets the count of Program List. */
	virtual int32 PLUGIN_API getProgramListCount() override { return 1; } // number of program lists. Always 1.

	/** Gets for a given index the Program List Info. */
	virtual tresult PLUGIN_API getProgramListInfo(int32 listIndex, ProgramListInfo& info /*out*/) override
	{
		if (listIndex == 0)
		{
			info.id = kRootUnitId;
			info.name[0] = 0;
//			info.programCount = (Steinberg::int32) factoryPresetNames.size();
			info.programCount = (Steinberg::int32) presets.size();
			return kResultOk;
		}
		return kResultFalse;
	}

	/** Gets for a given program list ID and program index its program name. */
	virtual tresult PLUGIN_API getProgramName(ProgramListID listId, int32 programIndex, String128 name /*out*/) override
	{
		const int kVstMaxProgNameLen = 24;
//		if (programIndex < (int32)factoryPresetNames.size())
		if (programIndex < (int32)presets.size())
		{
//			for (int i = 0; i < kVstMaxProgNameLen && i <= (int)factoryPresetNames[programIndex].size(); ++i)
			for (int i = 0; i < kVstMaxProgNameLen && i <= (int)presets[programIndex].name.size(); ++i)
			{
//				name[i] = factoryPresetNames[programIndex][i];
				name[i] = presets[programIndex].name[i];
			}
			return kResultOk;
		}
		return kResultFalse;
	}

	/** Gets for a given program list ID, program index and attributeId the associated attribute value. */
	virtual tresult PLUGIN_API getProgramInfo(ProgramListID listId, int32 programIndex,
		CString attributeId /*in*/, String128 attributeValue /*out*/) override
	{
		return kResultFalse; // no idea what this is for.

		//attributeValue[0] = 0;
		//return kResultOk;
	}

	/** Returns kResultTrue if the given program index of a given program list ID supports PitchNames. */
	virtual tresult PLUGIN_API hasProgramPitchNames(ProgramListID listId, int32 programIndex) override { return kResultFalse; }

	/** Gets the PitchName for a given program list ID, program index and pitch.
	If PitchNames are changed the Plug-in should inform the host with IUnitHandler::notifyProgramListChange. */
	virtual tresult PLUGIN_API getProgramPitchName(ProgramListID listId, int32 programIndex,
		int16 midiPitch, String128 name /*out*/) override {
		return kResultFalse;
	}

	// Parameter overrides.
	int32 PLUGIN_API getParameterCount() override
	{
		return static_cast<int>(vst3Parameters.size());
	}
	tresult PLUGIN_API getParameterInfo(int32 paramIndex, ParameterInfo& info) override;
	tresult PLUGIN_API getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string) override;
	tresult PLUGIN_API getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized) override
	{
		if (auto p = getDawParameter(tag); p)
		{
			valueNormalized = p->convertNormalized(p->stringToNormalised(ToWstring(string)));
			return kResultOk;
		}

		return kInvalidArgument;
	}
	ParamValue PLUGIN_API normalizedParamToPlain(ParamID tag, ParamValue valueNormalized) override
	{
		if (auto p = getDawParameter(tag); p)
		{
			return p->normalisedToReal(p->convertNormalized(valueNormalized));
		}

		return 0.0;
	}
	ParamValue PLUGIN_API plainParamToNormalized(ParamID tag, ParamValue plainValue) override
	{
		if (auto p = getDawParameter(tag); p)
		{
			return p->convertNormalized(p->RealToNormalized(plainValue));
		}

		return 0.0;
	}
	ParamValue PLUGIN_API getParamNormalized(ParamID tag) override
	{
		if (auto p = getDawParameter(tag); p)
		{
			return p->convertNormalized(p->getNormalized());
		}

		return 0.0;
	}

	// units selection --------------------
	/** Gets the current selected unit. */
	UnitID PLUGIN_API getSelectedUnit() override { return 0; }

	/** Sets a new selected unit. */
	tresult PLUGIN_API selectUnit(UnitID unitId) override { return kResultOk; }

	/** Gets the according unit if there is an unambiguous relation between a channel or a bus and a unit.
	This method mainly is intended to find out which unit is related to a given MIDI input channel. */
	tresult PLUGIN_API getUnitByBus(MediaType type, BusDirection dir, int32 busIndex,
		int32 channel, UnitID& unitId /*out*/) override
	{
		unitId = 0;
		return kResultFalse;
	}

	/** Receives a preset data stream.
	- If the component supports program list data (IProgramListData), the destination of the data
	stream is the program specified by list-Id and program index (first and second parameter)
	- If the component supports unit data (IUnitData), the destination is the unit specified by the first
	parameter - in this case parameter programIndex is < 0). */
	tresult PLUGIN_API setUnitProgramData(int32 listOrUnitId, int32 programIndex, IBStream* data) override { return kResultOk; }

	void ResetProcessor() override;

	// Presets
	std::wstring getNativePresetExtension() override
	{
		return L"vstpreset";
	}
	std::vector< MpController::presetInfo > scanFactoryPresets() override;
	void loadFactoryPreset(int index, bool fromDaw) override;
	void saveNativePreset(const char* filename, const std::string& presetName, const std::string& xml) override;
	std::string loadNativePreset(std::wstring sourceFilename) override;

	void OnLatencyChanged() override;
	bool sendMessageToProcessor(const void* data, int size);

	MpParameter* nativeGetParameterByIndex(int nativeIndex)
	{
		assert(isInitialized);

		if (nativeIndex >= 0 && nativeIndex < static_cast<int>(vst3Parameters.size()))
			return vst3Parameters[nativeIndex];

		return nullptr;
	}

	MpParameter_native* makeNativeParameter(int ParameterTag, bool isInverted) override
	{
		auto param = new MpParameterVst3(
			this,
			static_cast<int>(vst3Parameters.size()),
			ParameterTag,
			isInverted
		);

		tagToParameter.insert({ ParameterTag, param });
		vst3Parameters.push_back(param);

		return param;
	}

	bool OnTimer() override;

	IWriteableQue* getQueueToDsp() override
	{
		return &queueToDsp_;
	}

};

}} // namespaces

#endif

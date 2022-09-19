#pragma once

#include <string>
#include "mp_api.h"
#include "RawView.h"

namespace SynthEdit2
{
	class IPresenter;
}

class IGuiHost2
{
public:
	virtual int32_t RegisterGui2(gmpi::IMpParameterObserver* /*gui*/) { return 0; }
	virtual int32_t UnRegisterGui2(gmpi::IMpParameterObserver* /*gui*/) { return 0; }
	virtual RawView getParameterValue(int32_t parameterHandle, int32_t fieldId = gmpi::MP_FT_VALUE, int32_t voice = 0) = 0;
	virtual void setParameterValue(RawView /*value*/, int32_t /*parameterHandle*/, gmpi::FieldType /*moduleFieldId*/ = gmpi::MP_FT_VALUE, int32_t /*voice*/ = 0) {}
	virtual void initializeGui(gmpi::IMpParameterObserver* gui, int32_t parameterHandle, gmpi::FieldType FieldId) = 0;
	virtual int32_t sendSdkMessageToAudio(int32_t handle, int32_t id, int32_t size, const void* messageData) = 0;
	virtual int32_t getParameterHandle(int32_t moduleHandle, int32_t moduleParameterId) = 0;
	virtual int32_t getParameterModuleAndParamId(int32_t parameterHandle, int32_t* returnModuleHandle, int32_t* returnModuleParameterId) = 0;
	virtual int32_t resolveFilename(const wchar_t* shortFilename, int32_t maxChars, wchar_t* returnFullFilename) = 0;
	virtual int32_t getController(int32_t handle, gmpi::IMpController** returnController) = 0;
	virtual void serviceGuiQueue() = 0;
	virtual void LoadNativePresetFile(std::string /*presetName*/) {}
	virtual void setMainPresenter(SynthEdit2::IPresenter* presenter) = 0;
};
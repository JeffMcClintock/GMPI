#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "base/source/fobject.h"
#include "DrawingFrame_win32.h"

class SEVSTGUIEditorWin : public Steinberg::FObject, public Steinberg::IPlugView
{
	GmpiGuiHosting::DrawingFrame drawingframe;
	class IGuiHost2* controller = {};
    int width, height;
    
public:
    SEVSTGUIEditorWin(IGuiHost2* controller, int width, int height);

	//---from IPlugView-------
	Steinberg::tresult PLUGIN_API isPlatformTypeSupported (Steinberg::FIDString type) SMTG_OVERRIDE { return Steinberg::kResultTrue; }
	Steinberg::tresult PLUGIN_API attached (void* parent, Steinberg::FIDString type) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API removed () SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API onWheel (float /*distance*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API onKeyDown (Steinberg::char16 /*key*/, Steinberg::int16 /*keyMsg*/,
	                              Steinberg::int16 /*modifiers*/) SMTG_OVERRIDE {return Steinberg::kResultFalse;}
	Steinberg::tresult PLUGIN_API onKeyUp (Steinberg::char16 /*key*/, Steinberg::int16 /*keyMsg*/, Steinberg::int16 /*modifiers*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API getSize (Steinberg::ViewRect* size) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API onFocus (Steinberg::TBool /*state*/) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API setFrame (Steinberg::IPlugFrame* frame) SMTG_OVERRIDE	{return Steinberg::kResultTrue;	}

	Steinberg::tresult PLUGIN_API canResize () SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API checkSizeConstraint (Steinberg::ViewRect* /*rect*/) SMTG_OVERRIDE	{return Steinberg::kResultFalse;}

	//---Interface------
	OBJ_METHODS (SEVSTGUIEditorWin, Steinberg::FObject)
	DEFINE_INTERFACES
	DEF_INTERFACE (IPlugView)
	END_DEFINE_INTERFACES (Steinberg::FObject)
	REFCOUNT_METHODS (Steinberg::FObject)
};

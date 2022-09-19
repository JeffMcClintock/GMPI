#include "SEVSTGUIEditorMac.h"
#include "adelaycontroller.h"

// without including objective-C headers, we need to create an SynthEditCocoaView (NSView).
// forward declare function here to return the view, using void* as return type.
void* createNativeView(void* parent, class IGuiHost2* controller, int width, int height);
void onCloseNativeView(void* ptr);

SEVSTGUIEditorMac::SEVSTGUIEditorMac(IGuiHost2* pcontroller, int pwidth, int pheight) :
controller(pcontroller)
, width(pwidth)
, height(pheight)
{
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorMac::attached (void* parent, Steinberg::FIDString type)
{
    nsView = createNativeView(parent, controller, width, height);

	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorMac::removed ()
{
    onCloseNativeView(nsView);
    
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorMac::getSize (Steinberg::ViewRect* size)
{
    *size = {0, 0, width, height};
	return Steinberg::kResultTrue;
}

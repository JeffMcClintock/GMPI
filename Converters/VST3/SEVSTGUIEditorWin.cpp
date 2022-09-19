#include "SEVSTGUIEditorWin.h"
#include "adelaycontroller.h"
//#include "JsonDocPresenter.h"

SEVSTGUIEditorWin::SEVSTGUIEditorWin(IGuiHost2* pcontroller, int pwidth, int pheight) :
controller(pcontroller)
, width(pwidth)
, height(pheight)
{
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::attached (void* parent, Steinberg::FIDString type)
{
#if 0
    auto presenter = new JsonDocPresenter(controller);
    drawingframe.Init(presenter, CF_PANEL_VIEW);
    presenter->RefreshView();

    // Ableton opens at the wrong size, then resizes to correct size. Which we handle in onSize()
    // Voltage Modular never sets the size. Have to just force it.
    const GmpiDrawing_API::MP1_SIZE_L overrideSize{ width, height };

    drawingframe.open(parent, &overrideSize);
#endif
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::removed ()
{
//    onCloseNativeView(nsView);
    
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::getSize (Steinberg::ViewRect* size)
{
    *size = {0, 0, width, height};
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SEVSTGUIEditorWin::onSize(Steinberg::ViewRect* newSize)
{
    drawingframe.ReSize(newSize->left, newSize->top, newSize->right, newSize->bottom);
    return Steinberg::kResultTrue;
}

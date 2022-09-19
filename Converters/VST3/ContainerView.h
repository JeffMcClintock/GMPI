#pragma once
#include <vector>
#include <memory>
#include "ViewBase.h"

class IGuiHost2;
namespace SynthEdit2
{
	class IPresenter;
}

namespace SynthEdit2
{
	// The one top-level view.
	class ContainerView : public ViewBase
	{
		std::string skinName_;

	public:
		ContainerView(GmpiDrawing::Size size);
		~ContainerView();

		void setDocument(SynthEdit2::IPresenter* presenter, int pviewType);

		int GetViewType()
		{
			return viewType;
		}

		IViewChild* Find(GmpiDrawing::Point& p);
		void Unload();
		std::string getSkinName() override
		{
#if 0
			if( viewType == CF_PANEL_VIEW )
				return skinName_;
			else
				return "default2";
#else
			return {};
#endif
		}

		virtual void Refresh(Json::Value* context, std::map<int, SynthEdit2::ModuleView*>& guiObjectMap_) override;

		// Inherited via IMpGraphics
		virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override;
		virtual int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;

		GmpiDrawing::Point MapPointToView(ViewBase* parentView, GmpiDrawing::Point p) override
		{
			return p;
		}

		bool isShown() override
		{
			return true;
		}
		
		void OnPatchCablesVisibilityUpdate() override;

		GMPI_QUERYINTERFACE1(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI, IMpGraphics);
		GMPI_REFCOUNT;
	};

}

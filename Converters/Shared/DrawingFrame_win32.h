#pragma once

/*
#include "modules/se_sdk3_hosting/DrawingFrame_win32.h"
using namespace GmpiGuiHosting;
*/

#if defined(_WIN32) // skip compilation on macOS

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <d3d11_1.h>
#include "DirectXGfx.h"
#include "TimerManager.h"
#include "conversion.h"
#include "xplatform.h"
#include "mp_sdk_gui2.h"
#include "gmpi_gui_hosting.h"
#include "GraphicsRedrawClient.h"

namespace SynthEdit2
{
	class IPresenter;
}

namespace GmpiGuiHosting
{
	// Base class for DrawingFrame (VST3 Plugins) and MyFrameWndDirectX (SynthEdit 1.4+ Panel View).
	class DrawingFrameBase : public gmpi_gui::IMpGraphicsHost, public gmpi::IMpUserInterfaceHost2, public TimerClient
	{
		std::chrono::time_point<std::chrono::steady_clock> frameCountTime;
		bool firstPresent = false;
		UpdateRegionWinGdi updateRegion_native;
		std::unique_ptr<gmpi::directx::GraphicsContext> context;
		gmpi_sdk::mp_shared_ptr<IGraphicsRedrawClient> frameUpdateClient;

	protected:
		GmpiDrawing::SizeL swapChainSize = {};

		ID2D1DeviceContext* mpRenderTarget;
		IDXGISwapChain1* m_swapChain;

		gmpi_sdk::mp_shared_ptr<gmpi_gui_api::IMpGraphics3> gmpi_gui_client; // usually a ContainerView at the topmost level
		gmpi_sdk::mp_shared_ptr<gmpi_gui_api::IMpKeyClient> gmpi_key_client;

		// Paint() uses Direct-2d which block on vsync. Therefore all invalid rects should be applied in one "hit", else windows message queue chokes calling WM_PAINT repeately and blocking on every rect.
		std::vector<GmpiDrawing::RectL> backBufferDirtyRects;
		GmpiDrawing::Matrix3x2 viewTransform;
		GmpiDrawing::Matrix3x2 DipsToWindow;
		GmpiDrawing::Matrix3x2 WindowToDips;
		int toolTiptimer = 0;
		bool toolTipShown;
		HWND tooltipWindow;
		static const int toolTiptimerInit = 40; // x/60 Hz
		std::wstring toolTipText;
		bool reentrant;
		bool lowDpiMode = {};
		bool isTrackingMouse = false;

	public:
		static const int viewDimensions = 7968; // DIPs (divisible by grids 60x60 + 2 24 pixel borders)
		gmpi::directx::Factory DrawingFactory;

		DrawingFrameBase() :
			mpRenderTarget(nullptr)
			,m_swapChain(nullptr)
			, toolTipShown(false)
			, tooltipWindow(0)
			, reentrant(false)
		{
			DrawingFactory.Init();
		}

		virtual ~DrawingFrameBase()
		{
			StopTimer();

			// Free GUI objects first so they can release fonts etc before releasing factorys.
			gmpi_gui_client = nullptr;

			ReleaseDevice();
		}

		// to help re-create device when lost.
		void ReleaseDevice()
		{
			if (mpRenderTarget)
				mpRenderTarget->Release();
			if (m_swapChain)
				m_swapChain->Release();

			mpRenderTarget = nullptr;
			m_swapChain = nullptr;
		}

		void ResizeSwapChainBitmap()
		{
			mpRenderTarget->SetTarget(nullptr);
			if (S_OK == m_swapChain->ResizeBuffers(0,
				0, 0,
				DXGI_FORMAT_UNKNOWN,
				0))
			{
				CreateDeviceSwapChainBitmap();
			}
			else
			{
				ReleaseDevice();
			}
		}

		void CreateDevice();
		void CreateDeviceSwapChainBitmap();

		void AddView(gmpi_gui_api::IMpGraphics3* pcontainerView)
		{
			gmpi_gui_client = pcontainerView;
			pcontainerView->queryInterface(IGraphicsRedrawClient::guid, frameUpdateClient.asIMpUnknownPtr());
			pcontainerView->queryInterface(gmpi_gui_api::IMpKeyClient::guid, gmpi_key_client.asIMpUnknownPtr());

			gmpi_sdk::mp_shared_ptr<gmpi::IMpUserInterface2> pinHost;
			gmpi_gui_client->queryInterface(gmpi::MP_IID_GUI_PLUGIN2, pinHost.asIMpUnknownPtr());

			if(pinHost)
				pinHost->setHost(static_cast<gmpi_gui::IMpGraphicsHost*>(this));
		}

		void OnPaint();
		virtual HWND getWindowHandle() = 0;
		void CreateRenderTarget();

		// Inherited via IMpUserInterfaceHost2
		virtual int32_t MP_STDCALL pinTransmit(int32_t pinId, int32_t size, const void * data, int32_t voice = 0) override;
		virtual int32_t MP_STDCALL createPinIterator(gmpi::IMpPinIterator** returnIterator) override;
		virtual int32_t MP_STDCALL getHandle(int32_t & returnValue) override;
		virtual int32_t MP_STDCALL sendMessageToAudio(int32_t id, int32_t size, const void * messageData) override;
		virtual int32_t MP_STDCALL ClearResourceUris() override;
		virtual int32_t MP_STDCALL RegisterResourceUri(const char * resourceName, const char * resourceType, gmpi::IString* returnString) override;
		virtual int32_t MP_STDCALL OpenUri(const char * fullUri, gmpi::IProtectedFile2** returnStream) override;
		virtual int32_t MP_STDCALL FindResourceU(const char * resourceName, const char * resourceType, gmpi::IString* returnString) override;
		int32_t MP_STDCALL LoadPresetFile_DEPRECATED(const char* presetFilePath) override
		{
//			Presenter()->LoadPresetFile(presetFilePath);
			return gmpi::MP_FAIL;
		}

		// IMpGraphicsHost
		virtual void MP_STDCALL invalidateRect(const GmpiDrawing_API::MP1_RECT * invalidRect) override;
		virtual void MP_STDCALL invalidateMeasure() override;
		virtual int32_t MP_STDCALL setCapture() override;
		virtual int32_t MP_STDCALL getCapture(int32_t & returnValue) override;
		virtual int32_t MP_STDCALL releaseCapture() override;
		virtual int32_t MP_STDCALL GetDrawingFactory(GmpiDrawing_API::IMpFactory ** returnFactory) override
		{
			*returnFactory = &DrawingFactory;
			return gmpi::MP_OK;
		}

		virtual int32_t MP_STDCALL createPlatformMenu(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformMenu** returnMenu) override;
		virtual int32_t MP_STDCALL createPlatformTextEdit(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformText** returnTextEdit) override;
		virtual int32_t MP_STDCALL createFileDialog(int32_t dialogType, gmpi_gui::IMpFileDialog** returnFileDialog) override;
		virtual int32_t MP_STDCALL createOkCancelDialog(int32_t dialogType, gmpi_gui::IMpOkCancelDialog** returnDialog) override;

		// IUnknown methods
		virtual int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface)
		{
			if (iid == gmpi::MP_IID_UI_HOST2)
			{
				// important to cast to correct vtable (ug_plugin3 has 2 vtables) before reinterpret cast
				*returnInterface = reinterpret_cast<void*>(static_cast<IMpUserInterfaceHost2*>(this));
				addRef();
				return gmpi::MP_OK;
			}

			if (iid == gmpi_gui::SE_IID_GRAPHICS_HOST || iid == gmpi_gui::SE_IID_GRAPHICS_HOST_BASE || iid == gmpi::MP_IID_UNKNOWN)
			{
				// important to cast to correct vtable (ug_plugin3 has 2 vtables) before reinterpret cast
				*returnInterface = reinterpret_cast<void*>(static_cast<IMpGraphicsHost*>(this));
				addRef();
				return gmpi::MP_OK;
			}

			*returnInterface = 0;
			return gmpi::MP_NOSUPPORT;
		}

		void initTooltip();
		void TooltipOnMouseActivity();
		void ShowToolTip();
		void HideToolTip();
		void OnSize(UINT width, UINT height);
		virtual bool OnTimer() override;
		virtual void autoScrollStart() {}
		virtual void autoScrollStop() {}

		auto getClient() {
			return gmpi_gui_client;
		};

		GMPI_REFCOUNT_NO_DELETE;
	};

	// This is used in VST3. Native HWND window frame.
	class DrawingFrame : public DrawingFrameBase
	{
		HWND windowHandle;
		HWND parentWnd;
		GmpiDrawing::Point cubaseBugPreviousMouseMove = { -1,-1 };

	public:
		DrawingFrame() :
			windowHandle(0)
		{
		}
		
		HWND getWindowHandle() override
		{
			return windowHandle;
		}

		LRESULT WindowProc(UINT message, WPARAM wParam,	LPARAM lParam);
		void open(void* pParentWnd, const GmpiDrawing_API::MP1_SIZE_L* overrideSize = {});
		void ReSize(int left, int top, int right, int bottom);
		virtual void DoClose() {}
	};
} // namespace.

#endif // skip compilation on macOS

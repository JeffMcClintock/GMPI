#include "UI.h"
#include "Drawing.h"

using namespace gmpi;
using namespace GmpiDrawing;

class AGraphicsGui final : public gmpi::View
{
public:

	ReturnCode OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override // perhaps: ReturnCode OnRender(Graphics& g)
	{
		Graphics g(drawingContext);

		auto textFormat = GetGraphicsFactory().CreateTextFormat();
		auto brush = g.CreateSolidColorBrush(Color::Red);

		g.DrawTextU("Hello World!", textFormat, 0.0f, 0.0f, brush);

		return ReturnCode::Ok;
	}
};

namespace
{
auto r = Register<AGraphicsGui>::withXml(R"XML(
<?xml version="1.0" encoding="utf-8" ?>

<PluginList>
  <Plugin id="JM Graphics" name="Graphics" category="GMPI/SDK Examples">
    <GUI graphicsApi="GmpiGui"/>
  </Plugin>
</PluginList>
)XML");
}
<img src="Docs/images/GMPI_Icon.png" width="128"/>

# GMPI
Generalized Music Plugin Interface

In the same vein as VST and Audio Unit plugins, GMPI is a plugin API for Software Instruments and Effects.

The GMPI project was started to gather the best parts existing specifications and bring them together into an easy to use, open, powerful, and cross-platform alternative to the proprietary standards offered by Steinberg, Apple, and other vendors.

GMPI was instigated by the MMA (MIDI Manufacturers Association) as a collaborative effort. This implementation of GMPI is not endorsed by the MMA, but we've endeavored to adhere to the specification as closely as is practical.

# Features

GMPI:
* Supports Instruments, Audio Effects, and MIDI plugins
* Has a permissive open-source license
* Provides all API's in both C++ and in plain 'C'.
* Has cross-platform support
* Supports 'extensions'. Anyone can add their own new features. 
* Supports sample-accurate automation
* Supports plugin latency compensation
* Supports 'silent' buffer optimisations
* Supports musical timing information 
* Supports polyphonic parameters
* Thread-safe by default
* Plugin meta-data is plain XML

# A clean, simple audio plugin API.

Other plugin APIs require a lot of confusing 'boilerplate' code in every plugin. Below are plugin source-code examples which roughly illustrate
 how much overhead is required by other formats compared to GMPI.

<img src="Docs/plugin_api_complexity.png" width="260"/>

GMPI plugins are simply easier to write.  See the full source code of the GMPI gain plugin in [Samples/Gain.cpp](Samples/Gain/Gain.cpp)

But don't be fooled by the simplicity, even this basic GMPI plugin supports sample-accurate automation. This is becuase GMPI provides
*sensible default behaviour* for advanced features. Sample-accurate MIDI and parameter automation is *built-in* to the framework. Yet you can easily override the defaults if you need to.

# Metadata in XML
Rather than writing a lot of repetitive code to describe the plugin, GMPI uses a concise plain text format (XML). This is more future-proof than the rigid fixed 'descriptors'
of other plugin APIs, because you can add new features or flags without breaking any existing plugins or DAWs. Here's the decription of the example gain plugin...

```XML
  <Plugin id="GMPI Gain" name="Gain" category="SDK Examples" vendor="Jeff McClintock" helpUrl="Gain.htm">
    <Parameters>
      <Parameter id="0" name="Gain" datatype="float" default="0.8"/>
    </Parameters>
    <Audio>
      <Pin name="Input" datatype="float" rate="audio" />
      <Pin name="Output" datatype="float" rate="audio" direction="out" />
      <Pin parameterId="0" />
    </Audio>
  </Plugin>
```
Every plugin has a unique-identifier (the 'id') this can be a URI, a GUID or as in this example, the manufacturer and plugins names. Then the XML lists the plugins parameters, and then it's I/O (audio and MIDI input and output channels). A plugin can have any number of audio connections, and any number of MIDI connections. The third pin provides access to the parameter.


# Silence Detection

Silence flags are a feature to allow plugins to communicate that an audio signal is silent.
The advantage to a plugin is that when it's input signal is silent - it may not have to do any work. 

Adding only a few lines of code to our example plugin will enable the silence-flag feature.

```C
	void onSetPins() override
	{
		// Pass through the silence-flag
		pinOutput.setStreaming(pinInput.isStreaming());
	}
```
What this does is communicate to the DAW, that if the plugins input signal is silent - then so is it's output.

Notice how when it's input is silent the plugin can choose to completely shut down, using absolutely no CPU at all (see below).
This is just one example of how GMPI takes away the drudgery for you by providing *sensible default behaviour* (which can still be customized anytime you need to).

<img src="Docs/images/SilenceDetection.gif" width="654"/>

# Thread safe by default

GMPI plugins by default require no locks (e.g. std::mutex) and require no atomic values (e.g. std::atomic) when communicating between the various components.
This is because the GMPI framework provides a wait-free, lock-free message-passing mechanism as part of the framework. This feature alone eliminates a large
class of common bugs which are found in audio plugins.

Parameter automation and notification to/from the DAW is handled automatically by the framework. i.e. the audio processor class will be notified of parameter updates only on the real-time thread, and the Editor/GUI will be notified only on the 'message' (foreground) thread.

How does it work?

GMPI plugins communicate via 'pins'. A pin is just a class that contains for example a parameter value and some methods to make it easy to use.
A pin can be hooked up to a parameter, and/or to the plugins Editor (GUI) (You specify the connection in the plugins XML).

Let's take the example of a plugin with a VU Meter (volume meter). In GMPI the value of the meter would be held by a pin. e.g. 

```C
// This is the real-time component
class MyPlugin: public AudioPlugin
{
	FloatOutPin pinVuOut;
```

and to pass the VU Meter value to a parameter, or to the GUI, you assign a value to the pin.

```C
// This is the real-time component
void MyPlugin::OnMeterUpdate(float level)
{
	pinVuOut = level; // that's it from the real-time side. The framework handles the rest.
```

In the Editor (GUI) of you plugin, you would likewise have a similar pin to receive the value. e.g.

```C
// This is the GUI component
void MyPluginEditor : public GuiBase
{
	FloatGuiPin pinMeterValueIn; // pin to hold the parameter value.
	
	MyPluginEditor::MyPluginEditor()
	{
		// You can assign an update-handler to the pin
		initializePin(0, pinMeterValueIn, static_cast<GuiBaseMemberPtr>( &MyPluginEditor::onMeterValueChanged ) );

```

To read the pin value in the GUI:

```C
// This is the GUI component
void MyPluginEditor::onMeterValueChanged()
{
	// receive a value from the real-time thread.
	float VuLevel = pinMeterValueIn;	// hands-off, no mutex.. no atomic.. no trash talk on the black-top.
	
	// now draw the update on the GUI...
```
There's not much more to it, the GMPI framework will even handle the notification to the DAW that a parameter changed.

# Detailed GMPI Specification

[detailed-specs](Docs/GMPI_Specs.md)

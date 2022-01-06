<img src="Docs/images/GMPI_Icon_full.png" width="128"/>

Generalized Music Plugin Interface

In the same vein as VST and Audio Unit plugins, GMPI is a plugin API for Software Instruments and Effects.

The GMPI project was started to gather the best parts of existing specifications and bring them together into an easy to use, open, powerful, and cross-platform alternative to the proprietary standards offered by Steinberg, Apple, and other vendors.

GMPI was instigated by the MMA (MIDI Manufacturers Association) as a collaborative effort. This implementation of GMPI is not endorsed by the MMA, but we've endeavored to adhere to the specification as closely as is practical.

# Features

GMPI:
* Supports Instruments, Audio Effects, and MIDI plugins
* Has a permissive open-source license
* Provides all APIs in portable 'C'
* No fees, contracts or NDAs
* Has cross-platform support
* Supports 'extensions'. Anyone can add their own new features
* Supports sample-accurate automation, tempo and song-position
* Supports plugin latency compensation
* Supports 'silent' buffer optimisations
* Supports musical timing information 
* Supports polyphonic parameters
* Supports MIDI 1.0 and MIDI 2.0 plugins
* Thread-safe by default
* A clean and bloat-free API surface.
* Plugin meta-data is plain XML

# A clean, simple audio plugin API.

Other plugin APIs require a lot of confusing 'boilerplate' code in every plugin. Below are source-code examples which roughly illustrate
 how much overhead is required by other formats compared to GMPI. The plugins are all simple gain plugins. 

<img src="Docs/plugin_api_complexity.png" width="260"/>

GMPI plugins are simply easier to write.  See the full source code of the GMPI gain plugin in [Samples/Gain.cpp](Samples/Gain/Gain.cpp)

But don't be fooled by the simplicity, even this basic GMPI plugin supports sample-accurate automation. This is because GMPI provides
*sensible default behaviour* for advanced features. Sample-accurate MIDI and parameter automation is *built-in* to the framework. And you can easily override the defaults when you need to.

# Metadata in XML
Rather than writing a lot of repetitive code to describe the plugin, GMPI uses a concise plain text format (XML). This is more future proof than the rigid fixed 'descriptors'
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
Every plugin has a unique-identifier (the 'id') this can be a URI, a GUID or as in this example, the manufacturer and plugins names. Then the XML lists the plugins parameters, and then its I/O (audio and MIDI input and output channels). A plugin can have any number of audio connections, and any number of MIDI connections. The third pin provides access to the parameter.

# Thread safe by default

With GMPI, all Processor and Editor methods are thread-safe. i.e. the DAW does not ever call GUI components from the real-time thread, or vica versa.
GMPI plugins by default require no locks (e.g. `std::mutex`) and require no atomic values (e.g. `std::atomic`) when communicating between the various components.
This is because the GMPI framework includes a wait-free, lock-free message-passing mechanism. This feature alone eliminates a large
class of common bugs which are found in audio plugins.

# Silence Detection

Silence flags are a feature to allow plugins to communicate that an audio signal is silent.
The advantage to a plugin is that when its input signal is silent - it may not have to do any work. 

Adding only a few lines of code to our example plugin will enable the silence-flag feature.

```C
	void onSetPins() override
	{
		// Pass through the silence-flag
		pinOutput.setStreaming(pinInput.isStreaming());
	}
```
What this does is communicate to the DAW, that if the plugins input signal is silent - then so is its output.

Notice how when its input is silent the plugin can choose to completely shut down, using absolutely no CPU at all (see below).
This is just one example of how GMPI takes away the drudgery for you by providing *sensible default behaviour* out of the box (which can still be customized anytime you need to).

<img src="Docs/images/SilenceDetection.gif" width="500"/>

# Detailed GMPI Specification

[detailed GMPI specs](Docs/GMPI_Specs.md)

[full working group discussion](https://www.freelists.org/archive/gmpi)

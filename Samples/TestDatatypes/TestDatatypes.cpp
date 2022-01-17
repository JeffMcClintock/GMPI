#include "AudioPlugin.h"

using namespace gmpi;

struct Gain final : public AudioPlugin
{
	AudioInPin pinAudioIn;
	MidiInPin pinMidiIn;
	FloatInPin pinFloatIn;
	IntInPin pinIntIn;
	BoolInPin pinBoolIn;
	StringInPin pinStringIn;
	BlobInPin pinBlobIn;

	AudioOutPin pinAudioOut;
	MidiOutPin pinMidiOut;
	FloatOutPin pinFloatOut;
	IntOutPin pinIntOut;
	BoolOutPin pinBoolOut;
	StringOutPin pinStringOut;
	BlobOutPin pinBlobOut;

	Gain()
	{
		initializePin(pinAudioIn);
		initializePin(pinMidiIn);
		initializePin(pinFloatIn);
		initializePin(pinIntIn);
		initializePin(pinBoolIn);
		initializePin(pinStringIn);
		initializePin(pinBlobIn);

		initializePin(pinAudioOut);
		initializePin(pinMidiOut);
		initializePin(pinFloatOut);
		initializePin(pinIntOut);
		initializePin(pinBoolOut);
		initializePin(pinStringOut);
		initializePin(pinBlobOut);
	}

	ReturnCode open() override
	{
		// specify which member to process audio.
		setSubProcess(&Gain::subProcess);

		return AudioPlugin::open();
	}

	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		auto input = getBuffer(pinAudioIn);
		auto output = getBuffer(pinAudioOut);

		// just pass audio through.
		for(int i = 0 ; i < sampleFrames ; ++i)
		{
			output[i] = input[i];
		}
	}

	void onSetPins() override
	{
		// pass-though non-audio data types.
		if (pinFloatIn.isUpdated())
		{
			pinFloatOut = pinFloatIn;
		}
		if (pinIntIn.isUpdated())
		{
			pinIntOut = pinIntIn;
		}
		if (pinBoolIn.isUpdated())
		{
			pinBoolOut = pinBoolIn;
		}
		if (pinStringIn.isUpdated())
		{
			pinStringOut = pinStringIn;
		}
		if (pinBlobIn.isUpdated())
		{
			pinBlobOut = pinBlobIn;
		}
	}
};

namespace
{
auto r = Register<Gain>::withXml(R"XML(
<?xml version="1.0" encoding="utf-8" ?>

<PluginList>
  <Plugin id="GMPI Datatypes" name="Test Datatypes" category="GMPI/SDK Examples" vendor="Jeff McClintock" helpUrl="Gain.htm">
    <Audio>
      <Pin name="Audio"  datatype="float" rate="audio" />
      <Pin name="MIDI"   datatype="midi"   />
      <Pin name="float"  datatype="float"  />
      <Pin name="int"    datatype="int"    />
      <Pin name="bool"   datatype="bool"   />
      <Pin name="string" datatype="string" />
      <Pin name="BLOB"   datatype="blob"   />

      <Pin name="Audio"  datatype="float"  rate="audio" direction="out" />
      <Pin name="MIDI"   datatype="midi"   direction="out" />
      <Pin name="float"  datatype="float"  direction="out" />
      <Pin name="int"    datatype="int"    direction="out" />
      <Pin name="bool"   datatype="bool"   direction="out" />
      <Pin name="string" datatype="string" direction="out" />
      <Pin name="BLOB"   datatype="blob"   direction="out" />
    </Audio>
  </Plugin>
</PluginList>
)XML");
}

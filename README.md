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
* Supports sample-accurate automation
* Supports plugin latency compensation
* Supports 'silent' buffer optimisations
* Supports musical timing information 
* Supports polyphonic parameters

# A clean, simple audio plugin API.

Other plugin APIs require a lot of confusing 'boilerplate' code in every plugin. Below are examples of
 how much of this overhead is required by other formats.

<img src="Docs/plugin_api_complexity.png" width="260"/>

GMPI plugins are simply easier to write.  See the full source code of the GMPI gain plugin in [Samples/Gain.cpp](Samples/Gain/Gain.cpp)

But don't be fooled by the simplicity, even this basic GMPI plugin supports sample-accurate automation.

# Detailed GMPI Specification

[detailed-specs](Docs/GMPI_Specs.md)

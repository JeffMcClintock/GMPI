# GMPI Requirements - final draft proposal

converted from: requirements.php,v 1.73 2005/04/05 05:55:46 thockin.  

By: Tim Hockin <tim AT hockin DOT org>  
Based on emails from the GMPI working group  
http://www.freelists.org/archives/gmpi  

This document is a draft proposal. It is the result of the GMPI working group, and is not the property of any person, organization, or company.  

Open questions are hi-lighted.  

## 0\. Introduction

GMPI stands for "Generalized Music Plugin Interface". GMPI is meant to become an industry standard interface for communication of audio streams, musical events, and control signals between logical units. Specifically GMPI is meant to define the interfaces between a "host" program and "plugins". A host is a program which provides the GMPI framework and an environment in which to load plugins. Plugins are self-contained "sub-programs" that provide some piece of functionality. Plugins are logically connected to each other and to the host to form a graph or network, through which data can flow. The interconnection of all the pieces is what makes up GMPI.  

Several plugin formats, both open and proprietary, exist today. These interfaces all do more or less the same things, but they are widely different and not always completely overlapping in what they can and can not provide. The GMPI project was started to gather the best parts of each existing interface and bring them together into an easy to use, open, powerful, and cross-platform interface.  

## 1\. Goals

The fundamental goals of GMPI must be simplicity and compatibility. A GMPI plugin that works in one host should work in ALL hosts (with the exception of hosts that intentionally disallow some plugin types). A GMPI host that can load one plugin should be able to load ALL plugins. These goals may come at a cost. As much as people enjoy the idea of having many options, sometimes too many options means too much complexity for little advantage. Every decision made in designing GMPI must be made with these goals in mind.  

The secondary goals of GMPI must be flexibiltiy and extensability. It seems at first that these fly in the face of the primary goals, but that is not necessarily the case. Decisions and design must be done in an attempt to balance the goals. A host should be relatively straight forward to write, but there must be room in the API for compatible future growth. A plugin should be relatively straight forward to write, but there must be room in the API to do things in the best way for the plugin.  

GMPI should always try to make things easy for the most people possible. Often this means making plugins simpler at the cost of more complexity for hosts. There will be far more GMPI plugins than hosts.  

## 1.1\. Target Applications

GMPI is mainly targeting the following types of applications:

*   Digital audio editors, including audio editing in a video context
*   MIDI/audio sequencers and soft-studios, both live-play (realtime) and offline
*   Mastering systems
*   Gaming systems

This is not meant to imply that GMPI won't be usable in other applications. It simply means that the specification should primarily target the listed applications.  

## 1.2\. Target Developers

GMPI must be made accessible to the following developer bases:  

*   Professional plugin developers, who currently develop for other plugin formats
*   Professional host developers, who currently support other plugin formats
*   Amateur and hobbyist developers who may or may not be familiar with other plugin formats
*   Academic and experimental music researchers

## 1.3\. Target Plugin Types

Plugins generally produce and/or consume audio data and/or control/music data. Control/music data are things like knob turns, musical note data, etc. Some examples of target plugin types include, but are not restricted to:  

*   Audio-in, Audio-out:

> Delays, reverbs, chorus, flanger, compressor, EQ, etc.

*   Control-in, Audio-out:

> Synthesizers

*   Control-in, Control-out:

> Quantizers, transposers, style generators, etc.

*   Audio-in, Control-out:

> Pitch to audio converters, pitch detectors, envelope followers, etc.

## 1.4\. Target Operating Systems

In general, GMPI must target all popular contemporary OSes. Specifically GMPI must focus on:  

*   Windows
*   macOS
*   Linux

## 2\. General Guidelines

Throughout all of the GMPI design, a few guidelines should be kept in mind.  

*   GMPI should make it easy for developers to do the right thing.
*   GMPI should be flexible and extensible, but not cumbersome.
*   Hard limits should be avoided, wherever possible.
*   Choices should be left to the developers, except where a choice puts the primary goals in jeopardy.
*   GMPI needs to be efficient in every possible way.

## 3\. Requirements

## 3.1\. Cross-Platform

*   **Req 1:** GMPI must be hardware architecture neutral.

*   **Req 2:** GMPI must fully support 32 bit and 64 bit processors.

*   **Req 3:** The GMPI API must be defined in ANSI C, with a clearly specified application binary interface (ABI) for each supported platform. Actual calling conventions may vary from platform but must be standardized within a platform. A GMPI SDK must be provided in C++ and C.

[More on this requirement](#ApiDefinedInAnsiC)

*   **Req 4:** The GMPI API must use ANSI C type names (from stdint.h) instead of platform specific type names (e.g. uint32_t instead of DWORD).

*   **Req 5:** The GMPI design group should research options for a cross-platform portability library for GMPI plugins. An existing library may be adopted if a suitable specification for this already exists, or a new specification may be drafted. This should be a separate specification from the GMPI API. Plugins that wish to be more portable can use this API instead of native APIs whenever possible. This API is not the host services API, which should handle certain host-specific service tasks.

[More on this requirement](#PortableRuntimeApi)

It is my opinion that most developer would want to use a method of porting their software that they're accustomed to.


*   **Req 6:** GMPI must support the nuances of the various platforms it supports. The varying areas of the specification should be grouped into profiles for each platform. Each GMPI plugin is built for a single profile. As the GMPI specification becomes more complete, the exact details of what this support entails will become clearer. Hosts may support multiple profiles.

[More on this requirement](#Profiles)

I do not have any knowledge on what these variations would be and how they should be modeled into the API.

Note that the API definition also uses the term 'Profile' but perhaps in a slightly different meaning (the runtime contract between host and plugin on basis of which services will be provided to one another).


*   **Req 7:** GMPI must define clear and easy-to-follow rules governing access to those system resources which must be shared between the plug-in and the host. Examples include the Resource Context on Macintosh and the FPU state on x86.

It is my goal to provide enough documentation to allow both host and plugin developers ample guidelines for implementing these API's correctly.


## 3.2\. Basics

*   **Req 8:** Plugins must be able to control other plugins. The actual control mechanism may involve host arbitration, rather than direct plugin to plugin links. This control includes things such as parameters and control signals as well as things like voice allocation.

*   **Req 9:** GMPI plugins must be able to have an arbitrary number of audio inputs and outputs, including none at all.

*   **Req 10:** GMPI plugins must be able to have an arbitrary number of control inputs and outputs, including none at all.

[More on this requirement](#SendingAndReceivingControlSignals)

## 3.3\. Sample Rate

*   **Req 11:** GMPI must support arbitrary sample rates. Hosts may be selective, as needed (e.g. limited by hardware). Plugins must be able to indicate non-support of a sample rate.

*   **Req 12:** Plugins must not be expected to support dynamic sample rates. The sample rate must be set very early in the life of the plugin and not changed.

## 3.4\. Realtime

*   **Req 13:** GMPI must be primarily aimed at realtime processing systems, but must also support offline processing.

[More on this requirement](#RealtimeSystems)

*   **Req 14:** The GMPI SDK must provide best practices and recommendations for realtime programming.

*   **Req 15:** There must be some way for plugins to identify themselves as potentially unsafe for realtime use. Plugins must be able to indicate that they run offline (as opposed to realtime).

*   **Req 16:** GMPI must provide a way for plugins to require multi-pass processing (offline only).

The current method of multi-passing the offline processor may not be adequate but, since we have a separate interface defined for this case - we are sure to find something that works.


*   **Req 17:** GMPI must include a way for the host to indicate to plugins whether they are running in realtime or offline mode. Plugins can ignore this information completely.

## 3.5\. Threading

*   **Req 18:** The host must be in charge of all thread synchronization. No plugin functions must be re-entrant or asynchronous. Only one DSP function (within a plugin) is called at a time from the host. The host may call functions from different threads, but never more than one at the same time. This means the host is the arbiter of all access to the DSP object from other objects.

TBD: It may be necessary, upon actual experience, to incur some asynchronicity. In that case, GMPI must still make it easy for developers to get right. Rules must be simple, and if locks are needed, the GMPI specification must define a simple abstraction.  

It is not clear where this asynchronicity is or will be needed.


## 3.6\. Host <-> Plugin Model

*   **Req 19:** GMPI plugins run in the host's memory address space.

*   **Req 20:** GMPI plugins must always present a GMPI native interface to the host. Beneath the GMPI native interface, plugins may perform non-native operations, provided they do not violate GMPI. Plugins which need to run non-natively (e.g. DSP accelerated plugins, networked plugins) require a native proxy to the GMPI host.

[More on this requirement](#ProxyPlugins)

*   **Req 21:** GMPI must process audio data in blocks of sample frames. The host determines the size of the blocks and specifies it at processing time. Blocks may be as small as 1 sample frame. Hosts must never try to process 0 sample frames.

Perhaps there should be a special channel that communicates this fixed length more explicitly.


## 3.7\. Host Services

*   **Req 22:** GMPI must provide a way for a plugin to obtain information about the host it runs in. This information must contain at least a unique host identification and a host version number. The specification team may find it useful to add a host-specific callback from the plugin to the host, which hosts and plugins can use for out-of-band interactions, such as plugins that only run in a specific host.

Because this API specification works with service interfaces, it is totally possible to setup proprietary communication between host and plugin. No extra facilities need to be defined. But it is not recommended to do so.


*   **Req 23:** There must be some mechanism for plugins to request host services. The host services must include (at least):
    *   helper threads
    *   memory management (including buffers and events)
    *   time conversion

[More on this requirement](#HostServices)

Terms as memory management are so general that it is impossible to know how this requirement was meant by the author(s). Currently it is interpreted as providing a service that can be used to intercept memory allocation (for instance during processing). This would allow the host to perform some safe and fast allocation in a pre-allocated block of continuous memory.


## 3.8\. Events

*   **Req 24:** GMPI must implement a time-stamped, sample-accurate event system. Events are how realtime signals, including control changes, are represented.

*   **Req 25:** All events bound for a plugin during a timeslice must be enqueued before that plugin is activated to process that timeslice. Events must never be delivered while a plugin is processing.

[More on this requirement](#JustInTimeEventDelivery)

*   **Req 26:** Event timestamps must be measured in sample frames. The final specification may define sub-sample resolution.

[More on this requirement](#Timestamps)

*   **Req 27:** Events must be delivered to plugins in temporal order.

[More on this requirement](#EventDeliveryOrdering)

The host can use the PluginContext to enqueue and sort these events.


*   **Req 28:** GMPI must support ramped or smoothed events. Ramping or smoothing must be supported on controls with a real number datatype, at minimum.

[More on this requirement](#RampedOrSmoothedEvents)

TBD: The current Event definition does not define a value. Perhaps we need a derived event definition for EventWithValue?


*   **Req 29:** GMPI must support grouping events into 'gestures'. It must be possible to include zero or more events for multiple controls in a gesture. Support for gestures is optional for all plugins. The host must handle the case of conflicting or overlapping gestures.

[More on this requirement](#Gestures)

I do not understand what is meant with gestures and why it would be useful. I am inclined to think it is a services provided by the host?


## 3.9\. Time

*   **Req 30:** GMPI must provide one master clock per graph. The GMPI master clock must be sample based and provide sufficient range to represent several hours of time at a high sampling rate (i.e. 64 bits). The final specification may define sub-sample resolution. Hosts may build multiple graphs with different master clocks, but all GMPI plugins have exactly one master clock

[More on this requirement](#MoreOnTimelines)

*   **Req 31:** GMPI must provide a music-time clock. Music time is dependent on several variables such as tempo and meter, and is measured in some musically useful unit, such as (bars, beats, ticks), or a subdivision thereof. There may be more than one such clock in a graph.

[More on this requirement](#MoreOnTimelines)

*   **Req 32:** GMPI must provide access to a system-global clock (UST) which is compatible with the OpenML definition of UST. Plugins must be able to determine the UST time for any sample frame with minimal jitter.

[More on this requirement](#MoreOnTimelines)

*   **Req 33:** GMPI must provide a way for plugins to be notified of changes to music-time variables, such as tempo and meter. Changes to these variables must be delivered as timestamped events. There may be more than 1 change to the same variable within a single processing cycle. The final specification may provide additional methods of accessing these variables, such as a semi-static map.

[More on this requirement](#TimelineSyncAndEvents)

*   **Req 34:** GMPI must provide a way for plugins to get relative musical timings (e.g. given a start time, accurately find out when the time is exactly 1 beat later), absolute musical milestones (e.g. accurately find the start of the next beat), and musical "phase" (e.g. how far into a beat is sample frame X).

*   **Req 35:** GMPI must provide a way for plugins to control music-time variables, such as tempo and meter. The final specification will determine the actual model for this. GMPI must at least equal existing plugin APIs in this respect.

[More on this requirement](#PluginsControllingMusicTime)

The host determines what plugin gets access to the service interface that allows for manipulating Music Time. This assignment can change at runtime.


*   **Req 36:** GMPI must provide a way for plugins to be notified of changes to the transport state (e.g. playing or paused).

[More on this requirement](#TimelineSyncAndEvents)

*   **Req 37:** GMPI must provide a way for plugins to be notified of changes in the playback location (e.g. loops or jumps).

[More on this requirement](#TimelineAndSync)

*   **Req 38:** GMPI must provide a way for plugins to accurately convert between the various timelines, and a TBD list of other time formats (e.g. SMPTE) within a processing timeslice.

## 3.10\. Audio I/O

*   Req 39: All GMPI audio must be PCM data.

*   Req 40: All hosts and plugins must support non-interleaved audio data.

TBD: Based on as-yet-unknown benchmarks, the requirments might be extended to include a per-plugin preference for interleaved data. This extension will be included if and only if sufficient net performance gains can be demonstrated.  

*   Req 41: The audio sample datatype a plugin uses must be determined by the plugin's GMPI profile.

*   Req 42: All connections between plugins must be bundles of channels with defined relationships to each other (e.g. a mono stream is one channel, while a stereo stream is a left channel and a right channel, and a 5.1 stream is a bundle of left, right, center, surround-left, surround-right, and LFE channels). A plugin may have any number of input or output bundles, including none at all.

*   Req 43: Audio streams may represent trivial encodings (e.g. 2 signals = left and right channels) or non-trivial encodings (e.g. 2 signals = 4 channels, encoded in LtRt).

*   Req 44: Connections must be handled atomically with respect to the bundle. All channels in a bundle are connected or disconnected together. Hosts may provide mechanisms for unbundling and re-bundling streams.

*   Req 45: In-place processing (reusing an input buffer as an output buffer) must be supported by GMPI. If both the plugin and the host support in-place processing, it may be used. Hosts and plugins that do not support in-place processing must not need to do anything extra.

*   Req 46: GMPI must provide a performance optimization mechanism for handling silent audio streams. This mechanism must allow plugins and hosts which are built to use this optimization to flag silent outputs and detect silent inputs without examining the buffer contents.

## 3.11\. Parameters

*   Req 47: GMPI must provide a way for plugins to expose an arbitrary set of explicitly typed parameters. Parameters must be managed by the host.

More on this requirement  

*   Req 48: GMPI must support at least the following parameter datatypes:

real number (determined by the plugin's GMPI profile)  
integer  
string  
filename  
enumerated list  
boolean value  
opaque data  
More on this requirement  

It may be desirable to provide distinct double-precision and single-precision real number types.  

It may be desirable to provide distinct 32 bit and 64 bit integer types, or even just 64 bit integers.  

It may be desirable to use the opaque type with some metadata as the de-facto extensible type. Otherwise we may want to consider defining some extensible datatype.  

*   Req 49: All parameters must have associated metadata (e.g. name, minimum value, maximum value). Metadata will vary depending on the parameter datatype. All parameters must provide a default value. Plugins should handle invalid parameters safely.

More on this requirement  

*   Req 50: All parameters must be able to use natural values.

More on this requirement  

*   Req 51: GMPI plugins must be able to provide a parameter-specific method for producing a string representation of a parameter.

More on this requirement  

*   Req 52: GMPI plugins must be able to provide a parameter-specific method for producing a parameter value from a string.

More on this requirement  

*   Req 53: GMPI plugins must be able to provide a parameter-specific method for incrementing/decrementing a parameter value from a given value. The increment/decrement operation must return the new value, but not change the current value.

*   Req 54: Parameters must be marked as either hidden or visible. Hidden parameters should not be represented in UIs. This parameter property may change over the lifetime of a plugin.

More on this requirement  

*   Req 55: Every parameter must be either stateful or non-stateful. This parameter property may not change over the lifetime of a plugin.

*   Req 56: All parameters must be automatable. Parameter automation is simply event playback.

*   Req 57: It must be possible to have many listeners for parameter changes (e.g. a GUI and a motorized MIDI controller). Managing listeners is the host's responsibility.

*   Req 58: Plugins must be able to expose inter-parameter linkages. For example, two parameters may be linked together or not, as determined by a third parameter.

More on this requirement  

*   Req 59: Plugins must be able to perform seemingly spontaneous parameter changes. For example, a plugin might morph all its parameters from one state to another based on a parameter input or a MIDI note.

More on this requirement  

*   Req 60: GMPI must support dynamic parameter sets. When the parameter set changes, the host must be notified by the plugin. Hosts must be able to deny a parameter set change.

More on this requirement  

*   Req 61: There must be a way for GMPI plugins to save and restore their parameter-set state with patches.

More on this requirement  

*   Req 62: GMPI must support grouping of associated parameters. Grouping may be arbitrarily deeply nested.

*   Req 63: GMPI must support multi-channel plugins. Each channel may have any number of parameters and/or IOs, and channels need not be symmetric. There may be an arbitrary number of channels per plugin. It must be possible for a host to save just a single channel's patch as well as the entire state of a plugin, which includes all channels.

More on this requirement  

*   Req 64: Parameters must be able to receive events from multiple senders. The host must handle the routing and merging of events.

## 3.12\. Control I/O

*   Req 65: GMPI must allow for any source of parameter-setting events to properly drive any target parameter in the graph (where datatypes allow), regardless of any differences between source and destination natural value ranges. Conversion to and from normalized values must be handled by the host, using source and destination value range metadata.

More on this requirement  

## 3.13\. Latency

*   Req 66: GMPI plugins must report their latency when queried by the host.

*   Req 67: Plugins must be able to change their latency, when allowed by the host.

More on this requirement  

## 3.14\. Persistence

*   Req 68: A plugin's state is defined to be the collection of the values of it's stateful parameters at a given instant. For a host to save the state of a plugin, it must query all the plugin's stateful parameters and save them. Likewise, to restore the state of a plugin, a host must write values to all of the plugin's stateful parameters.

More on this requirement  

*   Req 69: GMPI must define a simple file format for storing plugin state independently of any host or project. These files are presets. The preset file format must must include the values for all of a plugin's stateful parameters, thereby completely describing the plugin's state. The format must also include information to identify the plugin for which the state is intended. The format must be editable outside of any host for primitive types. Opaque types must be encoded into the files in a compatible way.

More on this requirement  

*   Req 70: GMPI must define a simple file format for storing a group of plugin preset files into a bank of presets. It should be simple to build a bank of presets from a set of single preset files. A bank file must also include information to identify the plugin for which the state is intended. Bank files must be able to contain an arbitrary number of presets.

More on this requirement  

## 3.15\. User Interfaces

*   Req 71: Plugins may have zero or more UIs, including GUIs. Each GUI may have 1 or more windows.

*   Req 72: Hosts must support plugins with no UI. They can use parameter and plugin metadata to automatically generate a UI. All GMPI hosts must support plugins with no UI.

*   Req 73: GMPI must define a simple in-process custom UI mechanism. Hosts are not required to support this, but graphical hosts are strongly encouraged to do so.

More on this requirement  

*   Req 74: GMPI must allow arbitrary toolkit (including platform-specific) UIs, in a similar manner to what VST and AU do today.

*   Req 75: GMPI must explore a mechanism for out-of-process UIs. Hosts are not required to support this, but are strongly encouraged to do so.

More on this requirement  

*   Req 76: GUIs are not part of a DSP plugin with private access to the DSP internals. All communication between UI and plugin is via standard control signals.

*   Req 77: GUIs must be able to set parameters, read parameters (value and metadata), be notified when a parameter changes, store and recall programs, tell the host to load and save banks/patches, and tell the host to apply "MIDI learn" to a parameter.

*   Req 78: Plugin GUIs must have the option to be resizeable.

*   Req 79: Plugin GUIs must be able to expose static and configurable keybindings to the host.

## 3.16\. MIDI

None of the below requirements should be met by handicapping GMPI. In particular, MIDI has well-known limitations (integer field sizes, message model, etc.) and those limitations should not be taken as bounds on GMPI. The GMPI system can and should provide richer and more detailed information than MIDI is capable of. However GMPI must still interoperate losslessly with MIDI.  

All references to "MIDI" in this section refer to the MIDI 1.0 standard, as defined by the MMA.  

*   Req 80: Hosts are not required to support any external music control protocol, including MIDI. However, it must be possible to create a host in which users can control plugins from arbitrary external MIDI sources, such as hardware MIDI controllers or other MIDI software. This includes (but is not limited to) playing notes, setting parameters, host-based MIDI learn (click-and-wiggle) and system exclusive (SysEx) messages.

More on this requirement  

*   Req 81: It must be possible for plugins to send MIDI to arbitrary desitinations outside the host (though the in-graph data need not necessarily be pure MIDI). Plugins must not need to use platform-specific MIDI I/O APIs to achieve this.

*   Req 82: It must be possible for plugins to act as MIDI receivers, processors, and/or senders (though the in-graph data need not necessarily be pure MIDI).

More on this requirement  

*   Req 83: It must be possible for a GMPI plugin to act as a proxy for a hardware MIDI device (though the in-graph data need not necessarily be pure MIDI).

More on this requirement  

*   Req 84: All MIDI-originated events must be timestamped on the same timeline as all GMPI events.

More on this requirement  

*   Req 85: The GMPI event system must be able to represent all MIDI messages without any loss of information, including message content and time. An initial proposal for a single, common event coding has been drafted.

More on this requirement  

*   Req 86: Plugins which intend their parameters to be driven by MIDI must be able to expose a mapping of MIDI messages to parameters. The map must be changeable at runtime, for example when the parameter set changes.

More on this requirement  

*   Req 87: Plugins with MIDI-driven parameters, where changes to one parameter_ automatically produce changes in other parameters, must use an Actor to properly model any parameter linkages.

## 3.17\. Instruments

*   Req 88: Instrument plugins should be fundamentally no different than effects plugins. Any plugin that implements the instrument API can be played as an instrument.

More on this requirement  

*   Req 89: GMPI must provide a note control mechanism. This must include at least the ability to turn specific notes on and off. GMPI note control must provide the ability to turn on the same note number more than once, and identify exactly which instance of a note an event is intended for.

*   Req 90: GMPI must support fractional pitch numbers.

More on this requirement  

*   Req 91: Voice management must be the domain of the instrument. All that the host sees is a voice ID. Operations on a voice always use the ID.

More on this requirement  

*   Req 92: GMPI must provide the ability for an instrument to define an arbitrary set of parameters that applies to each voice. These parameters must be able to indicate whether they apply only to the start of a voice (e.g. velocity), apply continuously to the voice (e.g. aftertouch), apply only to the end of a voice (e.g. release velocity).

More on this requirement  

## 3.18\. Plugin Files

*   Req 93: Plugins must be dynamically loadable in their host environments. This is a platform specific definition, and the GMPI specification must define the formats for supported operating systems. Dynamically loadable files (such as .dll or .so files) which contain GMPI plugins must be differentiable from non-GMPI shared object files.

*   Req 94: Each plugin (dynamically loadable) file may contain more than one plugin.

*   Req 95: A plugin is managed (possibly with other plugins) as part of a "bundle" that includes the shared object file(s) corresponding to the plugin(s), and any associated files. These files could be presets, graphics, help files, manuals, samples, etc. When installed on a system, all files in the bundle must be located in a single directory. The bundle directory may contain nested directories, but these directories are private to the bundle. A bundle directory must never contain other bundles.

*   Req 96: Installing a GMPI plugin must be possible by simply unpacking and copying the plugin bundle to a directory. Nothing more is required. Plugin vendors may require additional steps (such as registration or unlocking) for their own plugins to be functional.

*   Req 97: It must be possible for non-privileged users to install plugins for their own use on platforms which support non-privileged users.

*   Req 98: Enumerating plugins is done by recursively scanning 1 or more directories for GMPI plugin files. The GMPI spec or implementation groups may decide to provide for static metadata outside the shared object. If the plugin bundle includes static metadata, hosts should use that metadata. If the plugin bundle does not include static metadata, the plugin must be probed for metadata.

*   Req 99: Plugins may self-categorize into a set of GMPI defined categories via their metadata.

*   Req 100: It must be possible to have two plugin files with the same file name in two different directories. For example, it must be possible to have two files, foo/demoplugin.dll and bar/demoplugin.dll in the GMPI plugin search path, and the host must be able to differentiate the files.

## 3.19\. Copy Protection

*   Req 101: GMPI should allow for copy-protected plugins, but should show preference for copy protection schemes that will allow metadata to be accessed without requiring access rights.

*   Req 102: Any GMPI copy protection must be optional for all plugins. Plugins which do not want copy protection must not be impacted.

## 3.20\. Localization

*   Req 103: GMPI plugins must be localizable. The host must expose the current language preferences to plugins.

*   Req 104: All user-visible plugin strings must be localizeable, with the corollary that all identifiers that are used by the host must stay consistent. For example, a parameter might have the name 'FOO' which is constant, and a display name, "Foo Frequency" which is localizable.

*   Req 105: All parameter data which is saved as part of a project must not be localized, which makes saved state locale-independent.

*   Req 106: GMPI must define a common string encoding.

More on this requirement  

## 3.21\. API Issues

*   Req 107: The GMPI API must be versioned. Version information must be presented in a way that allows hosts to query a plugin's GMPI version and vice versa.

*   Req 108: Plugins must have a well-defined version code, and the API must provide a simple way of decoding and comparing version numbers of plugins.

*   Req 109: Plugins must be uniquely identifiable. GMPI must not rely on a central authority for ID assigments.

More on this requirement  

## 3.22\. Wrappers

*   Req 110: GMPI must support wrapper plugins for other plugin APIs. At minimum, the following plugin APIs must be wrapable: VST(i), DirectX, AudioUnits, LADSPA.

*   Req 111: Wrapper plugins must present the wrapped plugins as native plugins to the host.

More on this requirement  

## 3.23\. Results

*   Req 112: The SDK must contain:

full GMPI documentation, including the specification and developer documentation  
GMPI header file(s), which can be included in a plugin or host project  
sample plugins demonstrating various features and details of the GMPI specification  
sample host code  

*   Req 113: The GMPI SDK must be released under a license with similar terms to the BSD license. The SDKs must be freely available in source form to any interested party. The example code and plugins must be able to be included in open source and closed source applications with no licensing fees or restrictions of any sort.

*   Req 114: The GMPI SDK must provide a tool or suite of tools to perform basic plugin validation.

## 4\. Justifcations, Clarifications, and Spec Fodder

During requirements gathering, much more detail was presented than the requirements alone can capture. In order to not lose the wealth of information, ideas, and opinions, they are presented here. Some of these sections are justifications for requirements, some are clarifications and examples of the effect of requirements, and some fairly evolved texts which could easily be the basis for sections of the GMPI specification.  

<a name="ApiDefinedInAnsiC" target="_blank"></a>

## 4.1\. API Defined in ANSI C

Because GMPI is supposed to be the universal plugin API, it must be compatible with many hosts and many platforms. ANSI C and it's associated tools are well understood, well tested, and are available on virtually every platform on the planet. Further, it is possible to encapsulate a C API into just about any other programming language.  

Being written in C does not mean that modern programming practices should not be used. GMPI can use object-oriented techniques, or whatever development models the implementors see fit.  

The GMPI SDK is the example code that plugin developers can use use to build GMPI plugins and hosts. The SDK must be provided in C++ and C.  

<a name="PortableRuntimeApi" target="_blank"></a>

## 4.2\. Portable Runtime API

By providing an API for accessing common system services, such as memory and filesystems, we can reduce the likelihood of a plugin not being portable to a different OS. Specifically, the areas targeted by this API should be the very areas where platforms differ. Other applications, such as Netscape/Mozilla have undertaken similar levels of cross-platform compatibility and have implemented APIs that could possibly meet our needs. Some potential solutions include the Netscape Portable Runtime , the Apache Portable Runtime, GLib, and POSIX.  

<a name="Profiles" target="_blank"></a>

## 4.3\. Profiles

Because it is impossible to design a single system that is be appropriate for all GMPI target platforms and still provide sufficiently high performance, it is expected that multiple 'GMPI profiles' (variants of GMPI) will need to be created. All GMPI implementations must have as much in common with one another as possible, and the profiles should describe only the set of things that vary from platform to platform. Every GMPI plugin must target a specific 'GMPI profile'. The full list of things that can vary per profile is TBD, but should include the audio sample datatype (e.g. some target platforms will use fixed-point or integer IO, when floating point processing is impossible or impractical).  

It should be possible to use plugins from two different profiles in the same graph, if a host allows it. Hosts can support more than one GMPI profile natively, or they can use wrapper plugins. Wrapper plugins can translate from the host's native profile to a different profile on a per-plugin basis, or on a sub-graph basis. Details of sub-graph plugins such as nesting depth and parameters should be handled by the final specification.  

<a name="SendingAndReceivingControlSignals" target="_blank"></a>

## 4.4\. Sending and Receiving Control Signals

GMPI plugins obviously need to be controlled, somehow (e.g. MIDI controllers, automation). GMPI plugins also need to be able to control other plugins. For example, an arpeggiator might need to control an instrument, or an envelope follower might control some other knob. The end result is a very modular system. Even a sequencer or timeline controller might be a plugin.  

<a name="RealtimeSystems" target="_blank"></a>

## 4.5\. Realtime Systems

GMPI should be designed with realtime or near-realtime hosts in mind. Plugins that work in realtime should work equally well in non-realtime or offline environments. Some plugins may not be suitable for realtime use. There may be potential features that offline-only plugins could benefit from. We should listen to those concerns, but keep the primary goal in mind.  

GMPI does not provide any realtime guarantees, but we need to make it easy for developers to do the right thing to provide glitch-free operation. That involves mostly simple API design, complete documentation, and good example code in the SDK.  

<a name="ProxyPlugins" target="_blank"></a>

## 4.6\. Proxy Plugins

It would be a mistake to not support existing technologies like DSP accelerated plugins and budding technologies like networked plugins. GMPI needs to enable these types of plugins and others, especially the ones we haven't thought up, yet. To do this without burdening the API with extra complexity, the GMPI working group reached the conclusion that these plugins can all run, but the host should be totally unaware of the non-native side of them. This makes for a very future-proof system. To make this work, these plugins must present a GMPI-native plugin interface and act as a proxy to the non-native operations. The host interacts purely with the GMPI interface.  

This seems to solve the problems presented. However, the GMPI specification and implementation need to be aware of these cases, and must be prepared to offer simple solutions to any further issues that arise. Native GMPI plugins must not pay the penalty for the quirks of these fringe requirements, but we must try to accommodate the requirements, none the less.  

<a name="HostServices" target="_blank"></a>

## 4.7\. Host Services

Along with the portable runtime environment, which abstracts platform-specific issues, there needs to be a host-specific abstraction. Hosts may choose to provide their own versions of some key routines, in order to keep better control. For example, a host might implement a realtime-safe memory allocator, or it may simply pass memory allocations on to the OS.  

<a name="JustInTimeEventDelivery" target="_blank"></a>

## 4.8\. Just-In-Time Event Delivery

Because GMPI is targeting realtime applications, it is impossible to know for certain what will happen in the future. Even sequenced events can change while the graph is processing. The results of the GMPI working group discussions suggest that GMPI graphs do not have a concept of future. Events are delivered just before the timeslice in which they are due. Any plugin that wants to queue events into the future needs to do it's own queueing, and must decide how to handle queued events in the event of the timeline changing (e.g. the tempo might change or the play position might jump forward or backwards, or even stop altogether). The SDK might contain example code for this.  

<a name="Timestamps" target="_blank"></a>

## 4.9\. Timestamps

Based on GMPI working group discussions, the preferred timestamp semantic is a sample frame offset from the start of the time slice. This allows the timestamp to be a 32 bit number, which is much easier for 32 bit processors to use efficiently than 64 bit numbers. This is a reccomendation. The spec team may choose to specify otherwise  

<a name="EventDeliveryOrdering" target="_blank"></a>

## 4.10\. Event Delivery Ordering

GMPI plugins should be able to rely on the assumption that their incoming events are already sorted by time. The GMPI 1.0 SDK should provide an implemention of an event queue that ensures events can easily be delivered in order.  

<a name="RampedOrSmoothedEvents" target="_blank"></a>

## 4.11\. Ramped or Smoothed Events

For some control signals point value changes are sufficient - the value of the control should change exactly when the timestamp indicates (plugins may want to do things such as using fast linear interpolation to avoid clicking, but the value of the control should reach the specified value in a very short amount of time).  

Sometimes, it may be possible to provide the plugin with some clues about event trends. For example, the host may know that the user has requested a linear transition of a parameter over period of time. Rather than sending the plugin a series of discrete events (between which the value is steppy), the host can send a single or small set of ramped or smoothed events.  

Ramped events indicate to the plugin that a control value is to change predictably over a period of time. The plugin can then handle the change smoothly, instead of as a series of point values (the zipper effect). A simple way to think of ramped events is a (duration, target-value) pair (though they may not actually be represented as such). The parameter should reach the target value over the duration. It is probably sufficient to make all ramps linear, though the specification or implementation teams may want to provide other shapes.  

To make connections between control outputs and control inputs simple, it probably makes sense to make point value events be a special case of ramped events (duration = 0). This allows all control outputs to generate ramped events without knowledge of their destination. If a control input receives a ramped event, but does not support ramping, the event can be turned into a point value by ignoring the duration, or can be downsampled into a series of smaller steps.  

As a potential alternative to ramps, the implementation may want to explore controls which receive buffers of values at the audio sample rate. Rather than sending a constant stream of audio-rate input to a control, these controls can receive timestamped events which include the audio-rate data only when there is a change being made. This provides the power of audio-rate controls, without having to keep the audio-rate input populated wastefully. This also provides the power to do complex ramp shapes with no additional plugin support. In fact, these audio-rate events can eliminate the need for ramps altogether. It was pointed out during discussions that this is an experimental feature, and as such might not be viable. The requirements group seemed to favor the ramping model, but the spec team has to make the decision.  

<a name="Gestures" target="_blank"></a>

## 4.12\. Gestures

Gestures are a logical grouping of a series of events. If events are explicitly joined by gestures, then they may be handled atomically. Hosts might use this for undo functionality, touch automation, or other features. Plugins do not need to do anything for gestures. It is expected that most plugins will not be gesture-aware themselves. Hosts can choose whether or not to be gesture-aware.  

As an example model, consider the case of a GUI plugin (or a touch-sensitive MIDI controller, or similar) controlling a DSP plugin.  

When the user clicks the GUI (or touches a control), the controlling plugin sends a gesture-start event with a unique (host-provided) ID to the DSP parameter being controlled.  
The host and/or plugin recognizes the gesture has started.  
The GUI or controller can now send events, which will be part of the gesture.  
When the user releases the GUI, the controlling plugin sends a gesture-stop with the same unique ID.  
The gesture ID is needed in the case of multiple DSP parameters in a single gesture. The above process would be done with the same gesture ID on the gesture-start sent to each parameter. The actual implementation details will be decided during the specification and implementation. The above is simply to clarify the idea.  

Any host which allow multiple event sources for one target needs to handle overlapping or simultaneous inputs. There are a couple viable options:  

1) Do nothing. Send all events to the plugin. The vast majority of plugins do not care about gesture-start and gesture-end, and will ignore them. If two sources fight, you will get a knob that jumps around. Plugins don't need to handle anything special, and ignore things they don't need.  

2) Act as a priority-based switch. Based on the source of inputs, decide which events make it to the plugin, and which get dropped. Plugins don't need to handle anything special, and ignore things they don't need.  

Both solutions are perfectly acceptable, and are not mutually exclusive. The SDK might have example code for this.  

<a name="MoreOnTimelines" target="_blank"></a>

## 4.13\. More on Timelines

During discussions with the GMPI working group, a more precise specification for required timelines was developed.  

Sample Time:  
measured in samples  
one clock per graph  
rate set by the host  
rate is immutable during the life of a plugin  
64 bit integer  
monotonic and linear  
never stops  
Sample time is an endless count of audio sample frames since some unknown starting point. It is the fundamental timestamp on all events. Most hosts will have one sample time clock, but they may provide multiple graphs with different sample time clocks. Absolute time does not stop with playback.  
Music Time:  
measured in ticks or other musical units  
N clocks per graph  
rate set by music time variables (e.g. tempo and meter)  
rate may change at any time  
representation is unclear, may be integral or real  
non-monotonic and non-linear  
stops and starts with the transport  
Music time represents a position within the musical score. It is only maintained when the transport is playing, and is undefined when the transport is not playing. Because music time is synced to the musical score, it may warp forward or backward with the playback location (e.g. in case of a loop or a user initiated jump). Hosts which do not really have the concept of musical time should provide a "fake" music time clock (e.g. 60 BPM).  
Unadjusted System Time (UST):  
measured in nanoseconds  
one clock per computer  
64 bit integer  
monotonic and approximately linear  
drift less than some defined amount  
never stops  
Streams with different fundamental timings, such as video and audio, can be synced by UST. All realtime events, such as MIDI input may need UST. The simplest UST implementation is for the host to provide (UST, sample time) pairs periodically (such as once per process cylce). Plugins can extrapolate the UST-to-sample time factor from those pairs.  
If there is a justifiable need for sub-sample accuracy, then sample frames should be divided into sub-samples, and sample time should represent the sub-sample counter.  

<a name="timelineUseCases" target="_blank"></a>

## 4.14\. Timeline Use Cases

In order to support the need for the multiple timelines, the GMPI working group defined a number of use cases to demonstrate various time-related needs.  

1\. Timestamped events  
A plugin is called to process a block of 512 samples. It is given the sample time of the first sample for that block. It might also receive some events, stamped with the exact sample time at which time they should be processed.  
Requires sample-based timing for events.  
2\. Tempo Sync  
A plugin wants to do a tempo-synced delay. The plugin needs to find out exactly how many sample frames constitute a given number of beats at the current tempo, and when the tempo changes.  
Plugins must be able to find out about tempo changes. With the tempo and sample rate, the plugin can convert tempo delays or real-time delays to sample counts.  
3\. Quantizing  
A plugin wants to quantize musical input to strict musical timing. The plugin needs to find out the current position within the current bar and beat, as well as the start of the next bar and/or beat. The plugin also needs to know something about the current time signature to make sense of beats and bars.  
Plugins must be able to find out the position of a sample frame within the bar and beat.  
Plugins must be able to find the edges of bars and beats.  
Plugins must be able to find out about tempo and meter changes.  
4\. Automatic accompaniment  
A plugin wants to act as an automatic drummer. It needs to know the tempo and the time signature and when they change. It needs to know exactly when bars and beats start, so it can lay down accurate drumming.  
Plugins must be able to find the edges of bars and beats.  
Plugins must be able to find out about tempo and meter changes.  
5\. Realtime tempo follower  
A plugin wants to analyze incoming audio and control the tempo and possibly meter of sequenced tracks. For example, a live guitarist and percussionist with sequenced synths. The live musicians can put in small tempo fluctuations to give the song a better live feel. The plugin can analyze the live audio and keep the sequenced synths in time.  
Plugins must be able to generate tempo and meter changes for other plugins.  
6\. MIDI (music sequence) tracks  
A plugin wants to act as a sequencer track for some other plugins. The plugin needs to trigger events at exact musical times (such as beats and bars). It needs to know the transport state (playing, paused, etc.) and when that state changes. It also needs to know when the playback location changes, such as when the user jumps to a different playback position. Changes to any other variables that control playback (such as shuttle speed) might need to be communicated to the plugin.  
Plugins must be able to find the edges of bars and beats.  
Plugins must be able to find out about tempo and meter changes.  
Plugins must be able to find out about playback location changes.  
Plugins must be able to find out about transport state and related changes.  
7\. Audio tracks  
A plugin wants to act as an audio track. The plugin needs to know when the playback location changes, so that it's playback head can be adjusted. It needs to know tempo and meter, and when they change. Changes to any other variables that control playback (such as shuttle speed) might need to be communicated to the plugin.  
Plugins must be able to find the edges of bars and beats.  
Plugins must be able to find out about tempo and meter changes.  
Plugins must be able to find out about playback location changes.  
Plugins must be able to find out about transport state and related changes.  

<a name="TimelineSyncAndEvents" target="_blank"></a>

## 4.15\. Timeline Sync and Events

To make it easy to sync, GMPI needs to provide a simple way for plugins to be notified when things like the tempo or the transport change. Given a few basic bits of information, plugins can track the timelines quite easily. It may also be useful to provide periodic resync notices (to prevent any drift), as well as milestones (such as beat or bar boundaries). These things are realtime events, and should be based on the GMPI event mechanism. See the section Well-Known Controls for a possible solution. The SDK should include example code for common timeline sync methods.  

<a name="PluginsControllingMusicTime" target="_blank"></a>

## 4.16\. Plugins Controlling Music Time

It must be possible for a plugin to change the music time clock for other plugins. VST provides this as an optional feature which almost no hosts implement. There are a number of ways this feature could work. A plugin might be able to send events directly to other plugins or the host might intervene. It may make more sense for the music time controller to provide a semi-static tempo map which can be modified by a plugin.  

Whichever method is used, GMPI should strive to avoid the same fate of this VST feature - almost no hosts support it, so no plugins use it. If that is to be the case, why specify it at all?  

<a name="WhatIsaParameter" target="_blank"></a>

## 4.17\. What is a Parameter?

A plugin's output is a function of it's input and some operational parameters. Parameters are the primary mechanism for dymanically changing the behavior of a plugin. Parameters may be directly tied to the plugin's internal algorithms or they may be more abstract.  

<a name="MoreOnParameterDataTypes" target="_blank"></a>  
4.18\. More on Parameter Datatypes  

It may be easier or cleaner to implement simple primitives, and map other datatypes onto those primitives in well-defined ways. For example, a boolean might be an integer primitive with a hint defining it as a boolean. Some non-primitive datatypes will be needed, such as filenames. A filename parameter might be a string parameter with a hint defining it as a filename, and some metadata about the search mask for the parameter (e.g. "*.wav").  

Opaque data parameters are opaque to the host. They must be represented as a pointer to a chunk of memory and a size. The host must not try to interpret opaque data. It may be necessary to add an 'undoable' flag to opaque parameters. Some parameters are chunks of data which make sense to undo, and some may not be.  

TBD: opaque parameters involve allocated memory. Perhaps they should provide clone and free() methods, such that an opaque chunk of data can be sent to two receivers and freed when done. Or perhaps some sort of reference counting should be used, so the last user will free the chunk when it's count drops to zero.  

<a name="ParameterMetadata" target="_blank"></a>

## 4.19\. Parameter Metadata

Each of the parameter datatypes has different bits of metadata required. Obviously, all parameters must identify their name and their datatype.  

Real numbers and integers should provide a minimum value and a maximum value. Enumerated lists need minimum and maximum values, but also need the names of the options. Strings might be more useful if they were hinted as to their contents. For example, if a string is hinted as a filename, the host might have a better clue about how to handle it. Boolean values might be hinted also. For example, a button that stays down when pressed (2 states) is different than a button that acts merely as a trigger before returning to its default state.  

The GMPI SDK should include examples of all parameter datatypes.  

<a name="NaturalParameterValues" target="_blank"></a>

## 4.20\. Natural Parameter Values

Parameters should be able to use natural values, as opposed to normalized values. Forcing parameters to use an arbitrary range of values and to map that range to it's natural values takes choices away from the developer for little gain. If a filter wants to use the integer values 20 to 22000 to represent it's range for cutoff frequency, it should be able to. Similarly, if it wants to use 0.0 to 1.0 to represent it's cutoff frequency, it should be able to.  

<a name="ParameterValueStrings" target="_blank"></a>

## 4.21\. Parameter Value Strings

Quite often a plugin will expose a parameter as a linear value range, such as 0.0-1.0 or 0-1000, but the implementation will use some non-linear mapping to produce the actual internal value. For example, volume might be a 0.0 to 1.0 range, but the actual dB value represented by the fader is non-linear. In these cases, plugins must be able to provide formatted strings for the host to use in place of the literal parameter value. Conversely, a user should be able to input a string and have the parameter be updated properly. Not all hosts will support direct text entry, and not all parameters will need more than trivial text-to-native conversions. The GMPI SDK should provide examples of the most basic converter routines.  

<a name="HiddenParanetersForBackwardsCompatibility" target="_blank"></a>

## 4.22\. Hidden Parameters for Backwards Compatibility

One issue raised during discussions with the GMPI working group was that of backwards compatibility. It was important that a new version of a plugin be able to function like an old version, if needed and designed as such, while still moving forward with features. The agreed upon solution was hidden parameters.  

Assume a plugin author wants to change the plugin in an incompatible way, for example taking one parameter and splitting it into two. He can create two new parameters, and keep the old one, but mark it as hidden. Any graph that references the old parameter will still work, but no new graphs will be created that use the hidden parameter. The plugin can internally provide compatibility with the old behavior by catching events sent to the old, hidden parameter.  

This allows for exactly as much compatibility as the plugin author cares to retain, with no API or performance penalty for other plugins that don't wish to retain compatibility.  

<a name="TheParameterPreProcessModel" target="_blank"></a>

## 4.23\. The Parameter Pre-Processor Model

In discussing requirements with the GMPI working group, a set of use cases was established for a carefully designed way of exposing parameter interactions to the host.  

1\. Linked parameters  
Two parameters, RATE and DEPTH, are independant of each other, unless a third parameter, LINKED, is set to true. When LINKED is true, RATE and DEPTH are joined at the same value, and move in lock step. When LINKED is set to false, RATE and DEPTH can be changed independently. All UIs must reflect this linkage.  
2\. Clipped parameters  
Two parameters, MIN and MAX, are independant of each other, unless a third parameter, LINKED, is set to true. When LINKED is true, MIN and MAX are locked to each other by their current offset, and move in lock step. MAX may not exceed 1.0, and MIN may not drop below 0.0.  
If MIN = 0.0 and MAX = 0.5 when LINKED is set to true, then the offset is 0.5\. Setting MIN to 0.3 would raise MAX to 0.8\. Setting MIN to 0.6 would attempt to set MAX to 1.1, which is out of bounds. MIN must be clipped at 0.5, and all UIs must reflect this clipping.  
3\. Morphed parameters  
The user establishes several patches and assigns them to specific MIDI notes. When the user presses the specified MIDI notes, the plugin begins to morph all the parameters to the assigned patch. If the user presses another note, the plugin begins to morph all the parameters from their current states to the assigned patch. UIs must reflect this ongoing morph.  
To conceptually handle these issues the idea of the "parameter actor" or "event pre-processor" was established. The final implementation is up to the spec and implementation teams, but the idea is simple. Plugins can optionally expose parameter event pre-processor methods. These methods encapsulate the logic of inter-parameter dependencies. Event senders call these methods synchronously, and are immediately alerted to things such as linked or clipped parameters.  

In addition to the pre-processor, plugins can provide a routine which is called during the processing loop, but runs before the plugin's own process routine. This helper routine can deliver events to the DSP plugin on a regular basis to handle self-automation tasks like morphs or continuous randomizations. Whether this helper actually functions as a helper plugin or merely as methods of a plugin will be decided by the spec or implementation teams.  

This model allows plugins to control all their parameter input logic without a custom UI and without violating any of the established GMPI conventions about events and event delivery. Any plugin which does not have special parameter input logic can bypass all of this. The SDK should contain examples of this.  

<a name="DynamicParameterSets" target="_blank"></a>

## 4.24\. Dynamic Parameter Sets

During discussion with the GMPI working group, dynamic plugin structures (parameters and IOs) was determined to be an elegant solution to a number of specific use cases. These use cases are included here.  

1\. Semi-modular plugin  
A synth has 4 oscillators, each of which may be one of three types, each with different parameter sets. The synth has 2 filters, each of which may be one of two types, each with different parameter sets. It has 2 effects, each of which may be one of 6 types, each with different parameter sets. It also has a long list of global parameters. See Linplug Albino for an example of such a beast. The resulting parameter set has all the parameters for all the modules. The custom UI can show only the relevant params, but the parameter list shows ALL the params.  
Identifying which params are relevant in the set of all params is difficult. Ideally, there would be a way to mark parameters that are currently relevant.  
2\. Modular plugin  
A modular environment has dozens of loadable modules, each of which has a unique parameter set. Each module may be loaded any number of times.  
The set of parameters is truly dynamic. All parameters should be automatable. Ideally, the set of audio inputs and outputs would be dynamic, too.  
3\. Multi-mode plugins  
A reverb can handle mono, stereo, or 5.1 inputs. Ideally, one plugin will work for all these IO types, dynamically.  
4\. Multi-stream plugins  
A compressor could affect any number of parallel streams based on a single master input. One plugin should be able to handle 1, 2, or 100 inputs of any stream type, determined at run time, if it wants.  

<a name="StroringPluginStructureWithaPatch" target="_blank"></a>

## 4.25\. Storing Plugin Structure with a Patch

During discussions with the GMPI working group, there were several alternatives proposed for how a plugin can store its structure (such as the parameter set, IO configuration, channels/slots etc.) with a patch. There might be a 'state header' which is saved/restored before the parameters. There might be a notion of 'properties' separate from 'parameters' (as in AU) which are saved/restored before parameters. There might just be an opaque parameter which holds this information.  

Any of these methods could work.  

<a name="MultiChannelPluginsAndParameterGroups" target="_blank"></a>

## 4.26\. Multi-Channel Plugins and Parameter Groups

Because MIDI interoperability is so important, the GMPI working group proposed several viable ways to handle multi-timbral plugins and parameter groups. It was proposed that simple MIDI-ike channels, where each channel has identical parameters) could be bettered by a more flexible system where each channel was potenially unique. This was dubbed the 'slot' model. The simplest use case for this is a GM-compatible plugin, which only needs identical channels. More complicated use cases, such as a mixer, require the more flexible model. To make it clear that each 'channel' need not be identical, the word 'channel' has been avoided in favor of the more general term 'slot'. A brief overview of some of the methods that were proposed is included here for completeness.  

1\. OSC-like naming  
If the parameter set is truly dynamic, then grouping and slot layout can be encoded into the parameter name. If we codify a slash (/) as a delimiter, then each parameter can be viewed as a path through a tree. Leaf nodes on the tree are parameters, and non-leaf nodes are groups. For example, if you have the following parameter set:  
/volume  
/osc/shape  
/osc/octave  
/filter/cutoff  
/filter/resonance  
You can assume that there are two groups, "osc" and "filter", each with two parameters, as well as a single top-level parameter, "volume".  
If you define a set of well known top-level nodes to represent things like slot grouping, you can turn the above example into something like:  
/master/volume  
/slot[0](https://gmpinet.codeplex.com/wikipage?title=0&referringTitle=GMPI%20Specifications)/osc/shape  
/slot[0](https://gmpinet.codeplex.com/wikipage?title=0&referringTitle=GMPI%20Specifications)/osc/octave  
/slot[0](https://gmpinet.codeplex.com/wikipage?title=0&referringTitle=GMPI%20Specifications)/filter/cutoff  
/slot[0](https://gmpinet.codeplex.com/wikipage?title=0&referringTitle=GMPI%20Specifications)/filter/resonance  
This clearly defines the plugin as having one slot, with a clearly defined slot structure. Plugins which want to be multi-timbral canhave multiple indices in the /slot[] array. In fact, slots can even have different structures.  
2\. Slots or Modules  
Instead of assuming the parameter set is an array of parameters, it can be considered an array of slots, each of which has an array of parameters. This allows a plugin to internally define common structures once, and saves the need for gratuitous string copying. It allows for an arbitrary number of slots, each of which may have an identical or a different structure. This combined with the slash-delimited names also provides arbitrary grouping.  

<a name="GenericControlOutputs" target="_blank"></a>

## 4.27\. Generic Control Outputs

It must be possible to produce plugins which provide generic control signals. These signals can then be tied to any parameter in the graph. This provides a whole new class of plugins, and a whole new level of flexibility. For example, it is possible to create a simple LFO plugin, a formulaic controller, or an image-processing controller. These arbitrary controllers can modulate anything. In order to do this, there must be some way to map the generic output range (essentially normalized output) to natural input ranges. A generic output might be unipolar (0.0 to 1.0) or bipolar (-1.0 to 1.0), which affects how it maps to an input.  

These generic controllers might be no more than parameter outputs, or they may benefit from some special flag or hint to identify them as generic.  

<a name="Latency" target="_blank"></a>

## 4.28\. Latency

It must be possible for plugins to change their reported latency. However, it may be too much to allow them to change it whenever they want. The following conditions are suggested for the specification of plugin latency reporting:  

Plugins may change their latency whenever they are queried by the host.  
Hosts may query or not query plugins as often as they see fit.  

<a name="SavingPluginState" target="_blank"></a>

## 4.29\. Saving Plugin State

For the majority of plugins, saving state does not require anything more than saving their parameters. Some plugins may require additional help. By allowing some parameters to be stateful and some to be stateless, we can better control what the host does and does not save and restore. If a plugin wants to save and restore some state that does not make sense to display as a parameter, it could provide a stateful but hidden parameter. If a plugin wants to save all of its state in a single opaque chunk, it can provide one stateful, hidden, opaque parameter, and all the other parameters can be stateless. This puts the choice with the developer.  

<a name="aPresetFileFormat" target="_blank"></a>

## 4.30\. A Preset File Format

A good solution to this requirement might be a standard XML DTD. XML can be arbitrarily nested, and is human editable. Primitive types can be represented as strings, and opaque data can be encoded to something like base64 for storage. If the implementation decides not to use XML, the GMPI working group recommends a text based format.  

<a name="aPresetBankFileFormat" target="_blank"></a>

## 4.31\. A Preset-Bank File Format

A good solution to this requirement might be a simple tarfile or zipfile of individual preset files. This makes a bank file easy to manage with standard existing tools. To accommodate the need for additional bank-global meta-data, perhaps a meta-data file should be included. The GMPI bank format should be the only sanctioned way of providing banks of presets, including "factory defaults".  

<a name="aCustomizedUiSpecification" target="_blank"></a>

## 4.32\. A Customized UI Specification

Allowing plugins to provide a customized UI should be kept simple. GMPI can not reasonably provide a full UI toolkit as part of it's cross platform capabilities. There are a number of UI toolkits that are already cross platform, and developers are encouraged to use those. A separate project may be formed to provide a widget library based on one of these toolkits, but that project is outside the scope of GMPI.  

A simple custom UI solution that was discussed is for the plugin to provide a UI definition file (possibly XML). The UI definition includes things like coordinates of various GUI controls, images to place at those coordinates, and GUI control to parameter mappings. The host could then render the UI from this information.  

This would allow a plugin to provide a nice looking, functional, but not totally controllable UI for plugins. Defining this UI system will involve defining a robust and featureful file format for the definition files, defining standard image formats for animations of controls, and more. It may be possible to develop a basic plugin to handle these UIs, which would only require platform specific drawing code. This should probably be an ancillary specification.  

<a name="UiDspCommunication" target="_blank"></a>

## 4.33\. UI - DSP Communication

A point of much debate with the GMPI working group was the topic of out-of-process UIs (separate processes running outside the host). Allowing UIs to run as separate processes allows plugin developers to do anything they like with the UI. With very little extra design, this same mechanism can be used for cross-network control of plugins. As long as DSP and UI are divorced, there is much flexibility.  

The debate centered around whether GMPI has any business defining the out-of-process communication protocol. It was argued that if GMPI does define it, then the market is open to many exciting things, such as GMPI UIs (including things like control surfaces) which can control any GMPI plugin remotely. OSC was proposed as a good, stable, existing standard that could be readily adapted to this purpose.  

It was countered, however, that this is a host-specific issue. GMPI should be defining the in-process plugin API. Not all hosts will want to allow out-of-process control, and those that do may well want to design it themselves. While OSC is a fine standard for such a task, hosts may want to do it their own way.  

The GMPI working group was not able to come to a final decision on agreement on whether, the GMPI spec should cover the inter-process or inter-machine transport mechanism, or whether this should be left to the discretion of host developers. The GMPI design and implementation teams should revisit this issue.  

<a name="MidiUseCases" target="_blank"></a>

## 4.34\. MIDI Use Cases

In order to best illustrate how MIDI and GMPI fit together, some use cases were created by the GMPI working group.  

Software version of a hardware synth  
A hypothetical instrument plugin is literally the same code that is running on a hardware synth, with a small GMPI wrapping. The developer wants to keep the core code as common as possible. This core code already handles MIDI and the developer does not want to change it. This plugin can elect to receive only MIDI-compatible messages.  
Click-and-wiggle MIDI learn  
In a hypothetical host, the user can define which MIDI messages affect a specific plugin instance and how. The user can select a parameter and instruct the host to do a MIDI learn. The user can then cause a MIDI message to be sent to the host, for example by moving a knob on a controller. The host can map that MIDI message to the specified parameter. The plugin does not need to handle raw MIDI, yet it can be driven by MIDI.  
SysEx compatibility  
A hypothetical instrument plugin is a software version of a hardware synth. The plugin can accept SysEx messages that are identical to the hardware synth, and behave similarly.  
MIDI processors  
A hypothetical plugin is a MIDI transposer. It receives MIDI-compatible messages, transposes them by some user-defined amount, and then emits MIDI-compatible messages.  
Hardware proxy  
A hypothetical plugin acts as a MIDI proxy for a hardware synth. The user can interact with the software interchangeably with the hardware. This plugin might choose to receive MIDI-compatible messages, or might provide a robust MIDI map. This plugin can also emit MIDI, which can be routed to the hardware synth.  
MIDI mapped parameters  
In a hypothetical host, the user can define which MIDI ports/channels are to be delivered to a specific plugin instance. The plugin publishes a MIDI map. When the host receives MIDI on those ports/channels, the host can convert the incoming MIDI into GMPI messages, which can be delivered directly. The plugin does not need to handle pure MIDI, yet it can be driven by pure MIDI.  

<a name="MidiTimestamps" target="_blank"></a>

## 4.35\. MIDI Timestamps

The GMPI workign group devised at least two potential models for MIDI in the GMPI system. One model involves GMPI and MIDI messages as a single timestamped message. Another model involved MIDI as a separate stream of data. In either case, MIDI must remain in time sync with all other GMPI events, so the timestamp on MIDI messages must come from the same timeline as other GMPI events.  

<a name="MidiTranscoding" target="_blank"></a>

## 4.36\. MIDI Transcoding

GMPI must include a single, clear, universal rule set for losslessly transcoding incoming MIDI messages to GMPI events, and for transcoding (with managed loss) outgoing GMPI events to MIDI messages. Transcoding must occur at the MIDI 1.0 message level, not the individual MIDI byte level.  

Depending on the eventual design of the pure GMPI note event(s), the relationship between MIDI messages and GMPI events may not necessarily always be 1:1\. For outgoing GMPI events that are transcoded to MIDI, the rules will need to exactly specify what truncation, rounding, etc. operations are needed when converting float or other high-resolution GMPI values to MIDI's limited integer fields. System Exclusive messages should always be passed verbatim, with no transcoding.  

<a name="MoreOnInstruments" target="_blank"></a>

## 4.37\. More on Instruments

Instruments are just ordinary plugins which happen to implement the necessary interfaces to be played as an instrument. They need to share all the same infrastructure and APIs as GMPI effects and other plugins. They are not a separate API, but just a sub-API of GMPI.  

Instruments are not necessarily output only. They can have audio inputs, and can act as effects or controllers, too. Defining the instrument interface can potentially be solved by Well-Known Controls. The GMPI SDK should include examples of instrument plugins.  

<a name="FractionalPitchNumbers" target="_blank"></a>

## 4.38\. Fractional Pitch Numbers

The GMPI working group seemed to converge on a model of MIDI-like pitch numbers, but represented as real numbers rather than integers. This is compatible with existing standards.  

<a name="MoreOnVoices" target="_blank"></a>

## 4.39\. More on Voices

A system must be developed for voice management that leaves the actual control of voices to the instrument, while giving the host enough flexibility. By using voice IDs that are not directly tied to physical voices, we can allow the same note to be triggered multiple times on an instrument, and still have each instance of the note be managed individually.  

<a name="PerVoiceParameters" target="_blank"></a>

## 4.40\. Per-Voice Parameters

GMPI must allow arbitrary parameters to apply to each voice. Traditional MIDI systems have a small number of parameters that can operate on each voice, such as velocity and aftertouch. Those limits do not need to apply to GMPI. By allowing the instrument to define parameters which can be set at the start of a note (such as velocity), parameters that can be modified during the lifetime of the note (such as aftertouch), and parameters which can be set at the end of a note (such as release velocity) we put the choice in the hands of the developer. MIDI interoperability might be provided by Well-Known Controls.  

<a name="StringEncoding" target="_blank"></a>

## 4.41\. String Encoding

All strings in GMPI should be encoded unicode strings. The encoding must be decided by the implementation team.  

<a name="UniqueIds" target="_blank"></a>

## 4.42\. Unique IDs

GMPI must avoid the need for a central ID authority or ID databases. Possible schemes for unique identifiers include:  

Vendor name string + vendor-assigned binary plugin ID  
Vendor-specific URI + vendor-assigned binary plugin ID  
Vendor-and-Plugin-specific URI  
A null unique identifier should be reserved. No vendor may release a plugin with the null identifier.  

<a name="PluginArguments" target="_blank"></a>

## 4.43\. Plugin Arguments

One way of making plugin instantiation more flexible might be to use plugin arguments as part of what defines a unique plugin. For example, a simple plugin would have no arguments - it can be identified by it's path/name/ID (however it is identified in the spec). A more complicated plugin, such as a wrapper plugin, is actually the same plugin DLL file, but with a different argument. Foo_Wrapper(a) is a fundamentally different plugin instance than Foo_Wrapper(b).  

<a name="WellKnownControls" target="_blank"></a>

## 4.44\. Well Known Controls

A number of requirements in GMPI fall into a pattern. They require or suggest some form of optional communication between the host and the plugins in realtime. GMPI's answer to realtime communication is events and control signals. By defining some well known control signals, plugins can indicate to the host that they implement some particular aspect of the GMPI API. For example, assume GMPI defines a common API for putting plugins into bypass mode. A plugin which provides the BYPASS control input is announcing to the host that it supports that API. Well known controls are a simple way of defining optional areas of the API while remaining completely extensible.  

Some of the ideas that have been suggested for well known controls include:  

a common way to put plugins into mute and bypass modes  
timeline sync  
tempo and meter changes  
transport position and transport mode changes  
MIDI compatibility (velocity, balance, pitch-bend, etc.)  
instruments (note on, etc.)  
latency reporting  
current quality mode

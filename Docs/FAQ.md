Generalized Music Plugin Interface Questions and Answers

https://github.com/angushewlett/plugnpain is from an ADC talk about plugin APIs (and their problems). It asks question that can be used to provide an insight and comparison of different approaches.


1. What is a plug-in, and how should the relationship between host, API and plug-in be framed at the level of design philosophy? Are your plug-ins subordinate to the host application, with the API/SDK enforcing this, or are they peers whose communication is mediated via your framework? Something more like the Eurorack hardware metaphor, or just 0.25in jacks to wire everything up?

Plugins and hosts have a symbiotic relationship. A host should provide much common functionality itself in order to make the plugin bloat-free, minimal and let the plugin developer focus on functionality and not boilerplate. for example GMPI hosts are required to provide a non-blocking thread-safe communication mechanism for communication between the plugin Editor and its Processor. This alone saves much developer effort and eliminates one of the common causes of plugin bugs and beginner-confusion

2. How do you handle metadata, fast scanning, multi-shells.. can a single system package (component, bundle, DLL etc.) serve up more than one plug-in object? Is that list of things static (within a given version release / binary checksum / last-modified date) or dynamic? Are different connection topology variants of the same device (mono, stereo, surround) different objects, or different configurations of the same object?

GMPI Metadata is held in a text-based XML format. This facilitates fast-scanning by allowing the DAW to read the metadata without having to instantiate a plugin instance. A single package can contain multiple plugins. This can be either static (read from the XML) or dynamic via the 'shell plugin' extension. Different topologies can be provided by either completely separate plugins, or the same plugin with different configurations.

3. How does your API cleanly handle error conditions that might prevent packages loading or instantiating objects, and capture and communicate those states in a way which is non-blocking and compatible with headless / command-line operation? Do you have guidelines to ensure that non-compliant communication of such error conditions (modal licensor dialogs during scan) constitutes a hard FAIL?

The API handles error conditions via standard return-code method. With headless plugins the host does not instantiate any GUI code, only the separate processor object. We have no guidelines on modal dialogs as such.

4. How do plug-in and host negotiate I/O semantics, both at instantiation and during runtime? Not just the number and type of I/O "pins", but the meaning of those pins, and valid (input, output) pairings/combinations/constraints. (Or: do you intentionally handball this to users, as audio jacks and XLR connectors did for many a year.)

Currently a package would list multiple configurations as separate plugins e.g. "Gain (mono)", "Gain (stereo)". Each with a different I/O configuration. The user/host would then select the appropriate configuration.

5. What are your editor (GUI) lifetime semantics relative to your main object; what are the allowed (m <-> n) configurations of plug-in and editor; to what extent are process separation and thread separation enforced? Is all transfer of state between audio object and GUI object mediated via the host, and how are potentially larger and more complex chunks of state shared (sample library information, DRM/licensing state etc..)

GMPIs main object is the "Controller", its task is to manage the plugin state. The Controller is instantiated first and destroyed last. The Editor and Processor objects are optional and are instantiated as needed. It's normal to instantiate these classes and destroy them as needed during a session. GMPI supports multiple simultaneous Editors and Processors. Synchronization is managed transparently by the DAW without any tricky plugin code. GMPI plugins editors and processors are by default completely fire-walled. The editor only deals with the main thread, and the processor deals only with the real-time thread. There is no need for any locks etc. No memory is shared (unless you want to). The host mediates all communication and supports the transfer of large chunks of data. For 'huge' data like sample-libraries the plugin can bypass the standard method and share the memory directly to save on copying.

6. How does API manage screen resolution negotiation, DPI management & real-time resizing?

The GMPI Drawing API is resolution independent, using 'DIPs' (Device Independent Pixels) to specify dimensions, font-size etc. The resizing extension is not implemented yet.

7. How does the API differentiate save-state chunks vs presets or programs - these are similar but not the same thing (often a plug-in might save extra information in a “host save” compared to a preset). Are MIDI program changes supported (real-time or deferred), is there a concept of edit buffers, what rules are applied to visible state consistency for state-change operations applied in a non-real-time and real-time thread?

GMPI Plugins are designed for precise and unambiguous state recall.  A preset is simply the complete current state. State data can be marked as 'non-stateful' if they apply only to the current session. The plugin and host share access to the state which eliminates the need for any explicit 'saveState()' or 'getState()' type of API. The plugin Editor and Processor are merely 'observers of the state' and are each notified safely on their own threads of any change in the state. These design decisions eliminates a source of complexity and bugs in plugins. For consistency, preset changes are communicated to the processor with sample-accurate updates. GMPIs philosophy is that MIDI program changes should not require explicit support but be translated by the host into a 'normal' preset change. But since GMPI supports MIDI input, this is ultimately the plugins decision.

8. How is preset enumeration defined? Are banks and indices a thing, who is responsible for maintaining preset databases, is there some sharing of "container" file format and metadata?

The idea that thousands of plugins should all separately reinvent preset management is... inefficient, Preset management is the hosts job.

9. What is the approach to memory management for variable-sized plug-in output data, particularly the Save Chunk and real-time MIDI output?

In general, the Processor should reserve memory ahead of time (not in the real-time context) to hold any large MIDI messages. These are passed to the host via pointers.

10. When to MIDI-bind parameters and when not to; how to clearly distinguish volatile “performance” data from persistent “parameter change” data. (For example, a plug-in is not expected to save pitch bend wheel state in a preset. Should mod-wheel performance data, or poly aftertouch, or some MPE gesture, be considered to “dirty” the state of a project/preset, and to flag the undo bit?). Clear responsibility between plug-in and host for controller binding, but in a way that doesn’t interfere with MIDI (or MPE) controller data that is being used for performance rather than parameter editing. What are the rules for state-change visibility when these messages may arrive on different threads?

Volatile performance data is stored as 'non-stateful' state. i.e. data that is retained for the duration of a session but not stored in presets. GMPI plugins support MIDI, MPE and MIDI 2.0 input directly so there is nothing surprising there regarding performance data. The host also can bind MIDI to parameters, any change to the state (parameters) is communicated independently to both the Editor and Process on their own thread.

11. Does your API precisely specify threading rules, in a way that makes it no harder than necessary to write correct lock-free code, or at least to allow the real-time part of plug-in audio engines to be implemented in a lock-free or near-lock-free manner?

GMPI has strict threading rules. No object (Editor, Processor) deals with the other ones thread. All communication is lock-free, race-free, non-blocking and provided by the host. Of course you are free to use additional threads for expert scenarios.

12. Does your API have a concept of (static or dynamic sized) multi-timbrality for plug-ins? Is the plug-in object a single logical target/component with linear indexing of parameters, or is there some kind of addressing hierarchy (homogeneous or heterogeneous), per the concept of elements/scopes in Audio Units?

The philosophy of GMPI is that a plugin should be a single logical target/component. If you need multi-timbrality, you should instantiate multiple plugins. But this is not enforced, go ahead and make multi-timbral plugins if you want to.

13. How is buffer sizing behavior, timing of buffer delivery etc. specified? (note USB audio cards and their habit of pulling unevenly sized buffers). Fixed size, variable size, powers of two, isochrony...

Buffers are any size greater than one sample. The host communicates the maximum expected 'block size' to the plugin but is allowed to pass fewer samples.

14. What is the spec for buffer sizing and clock behavior during preroll, at loop start and end points, audio tail handling (after end of timeline, and after playback stop).

GMPI plugins support sample accurate 'silence flags' to communicate to the host when the 'tail' of the audio has ended, and to allow the host to 'sleep' the plugin to save CPU when no audio is being processed.

15. When should plug-ins flush different bits of the audio engine (think soft flush for looping vs MIDI All Notes Off vs hard flush with buffer reallocation)

The Processor should be reset as a result of any change to the maximum buffer-size or sample rate. Additionally the host can communicate changes to the offline/online mode and to communicate the start of processing a clip or loop (in order to clear tails etc).

16. How is cooperative multi-threading supported at audio thread level, how do plug-ins and hosts to negotiate on core allocation, pinning etc. to avoid situations where plug-ins are loading the CPU heavily and not reporting it, fighting one another for cores etc., note bigcores vs littlecores. Can the API ensure that all threads are properly accounted for (to avoid multi-threaded hosts and multi-threaded plug-ins contending for the same secondary core)

There is no explicit API for cooperative multi-threading. However this could be implemented as a extension. (GMPI supports both plugin and host custom extensions)

17. Can non-atomic MIDI messages (NRPNs, SysEx streams, MPE note-ons) be packaged as an atomic event?

Yes, all events are sample-accurate. Any two or more events with the same time-stamp are considered 'atomic'

18. Is there a system for preloading / caching programs/presets, so that large state-changes can be applied without interrupting streaming/playback? 

GMPI Presets may be cached by the host. The plugin does not need to be aware of this.

19. Does the API provide event-based audio clocks (“sync pulses”) rather than polling / callback-based clocks? Due to PDC (plugin delay compensation), not all plug-ins in the same logical graph pull / process-buffer are necessarily running in the same sampletime context. Is there a specification for clock updates during looping playback, particularly when loop boundaries are not aligned with natural audio-callback buffer boundaries)?

All events are sample-accurate. Including BPM and musical time. Musical time is provided via sync pulses, not polled. This allows for seamless looping, scrubbing etc without any special handling. All signals are latency-compensated. MIDI, parameters, presets and musical time are always perfectly synced.

20. What is specified in relation to resource management / contention and OS access inc lifetime/cleanup? To what extent does the host try to provide/replace/mediate "operating system" services? (see also a11y/i8n/l8n and security context topics below).
    
The SDK provides a few helper classes for dealing with OS resources like timers and files. There is an extension for loading resources like images and samples so that one copy can shared. Anything more is outside the scope of GMPI.

21. What services are provided in relation to parameter layout semantics (e.g. control surface mapping “automap”, “scopes” in AU, Avid's page tables)?

Not currently supported.

22. Does the API provide any affordances for accessibility, internationalisation, localisation? (One valid answer: “we don’t solve this, use the OS services”, but should at least be a conscious decision).

GMPI string data uses the UTF encoding, the drawing API also uses UTF-8. We don't specifically handle localisation, but do have plans to support accessibility.

23. What support is built-in for wrappers/bridges supporting out-of-process operation (application to application), plug-ins operating either as a separate process individually, or in a collection of plug-ins that’s isolated from the main host data model & GUI?

GMPI is designed for process-separation. The editor and processor need not be in the same address space or on the same machine.

24. cf threading topic above, to what extent does your API specify mutexing / serial access (for non audio thread) - for many non-trivial plug-ins there are effectively two “serial” contexts (“real-time thread” and “everything else”) from an API perspective - even lightweight operations (get parameter value) are potentially unsafe in a dynamically configurable plugin. Is it better to put this responsibility (locking-correctness) on the host side given that there are fewer hosts with, on average, more competency in writing lock-correct code?

Locking-correctness is provided by the host. This is an essential core premise of GMPI. The master state of the plugin is always accessible to the host, even when the Processor and Editor are not even instantiated. This allows the host to start/stop the Processor at any time, yet place it back in the exact same state that it left-off at. Same with the GUI. This is superior to most other plugin APIs. Parameter and program-change events are all mediated by the host.

25. Data model state changes, ordering, visibility: both the UI thread (loadState, guiSetParameter…) and audio thread (parameter change event, program change event) can potentially update the master state. What is the correct behavior for a plug-in in the event of possibly conflicting messages? Where does the master “state” live (“DSP object”, “GUI object”, or another object which lives on the UI thread but is not the GUI)? How is the DSP object kept updated in cases where the audio thread isn’t active?

State lives in the 'Controller' object (in the UI thread, but not the GUI). All states changes go through this object and are then communicated to both the Editor and Processor.

26. “Parameters going through the host” (notification between plug-in’s GUI and DSP). Inevitably, in all but the most prescriptive APIs, there will be some aspects of state-change / state-messaging which do not pass through the host - even in a fully separated API, GUI and DSP will end up passing file URLs or other shared resource and then using them to transfer state. In which exact circumstances is “messages via the host” better, and why?

To rephrase the question: "Why can't filenames be as easy to pass to the Processor as parameters are?".  In GMPI, strings and filenames are first-class parameters. They are automatically included in presets (unless you opt-out) and automatically synced between the GUI and Processor.

27. What support if any is provided for “idle” UI time management - plug-in timer callbacks vs host “idle” timer callbacks etc.?

GMPI provides a helper class to provide a timer. This is independent of the API. 

28. Is there specific support for background thread management for plug-in worker tasks (low and high priority) - as distinct from multi-core audio processing (see point 15 above)? Low priority tasks e.g. preset database re-indexing, medium priority e.g. visualisers/analysers, high priority e.g. disk streaming?

Nothing specific.

29. Does the API enforce (or not) best practice in the design of plug-in state serialization, e.g. structure, versioning, portability.. a linear list of (0..1) parameters may be too basic for a lot of use cases, but binary opaque chunks are binary and opaque.

The API automatically serializes the state of float, int, bool, string, and binary chunks. Plugins do not need any specific 'save state' functionality. This is a huge time-saver.

30. Is there any mechanism for dealing with partial non-compliance during a transition from legacy APIs and hosts - sometimes it is better for things to work imperfectly as long as people know it’s gonna be imperfect. MIDI has been imperfect for almost 40 years…

All improvements to GMPI are provided via strictly typed 'extensions'. For example SynthEdit currently supports 1 current and 2 legacy graphics extensions. Extensions can be dropped, there is no requirement to support legacy APIs forever. Every API such as the Editor and Processor is an extension that could potentially be enhanced, replaced or dropped.

31. How is MIDI 2 supported?  Is native/raw MIDI2 data stream available by default, or only as a special-case, or not at all? As a large & complex spec it seems to have potential ramifications far larger than just “give me the data and let me get on with it”.

GMPI supports both MIDI 1.0 and MIDI 2.0, we have converter classes that give you the convenience of converting MIDI 1.0, and MPE to MIDI 2.0 so that you can choose to deal only with the one standard.

32. Rules around audio i/o buffer identities (the raw in-memory objects). Can in and out buffers be aliased / the same? Can two input (or output) buffers be the same? Can buffers in the frame be null pointers? What’s considered permissible if anything, other than normalized floats in-range? Denormals etc.?

By convention two or more inputs can share the same buffer. Outputs are distinct from input buffers. Buffers must not be null.

33. Does your API require C++ ABI compatibility? (Beware, keep things simple, use C ABI).

GMPI is a C ABI. The SDK is C++, but we provide a separate C SDK and plugin example written in C for masochists.

34. RTL object compatibility, both C++ (STL containers, streams) and c (FILE* and other handles). Don't assume plugin and host will be linked to same RTL instance or even the same version.

All communication with host is strictly via the C ABI with plain old C types.

35. Security contexts. How does your API manage potential differences in security context requirements (e.g. mic and camera access) between host and plug-in? Is there any mechanism for a plug-in to share state (both in memory and via persistent filesystem objects) between instances of itself that may be loaded in different application/host security contexts? For example making the same user-downloaded presets available in different applications.
    
We do often share data like wavetables and samples between plugin instances in the same address space. But not otherwise.

36. Binary platform nuances - universal binaries and Rosetta on OS X. ARM64X vs ARM64EC vs emulation for Windows-ARM. What does the API specify and are there specific transitional arrangements?

We currently ship plugins on Windows 32/64 and macOS Intel/ARM. The API is neutral regarding the PCU architecture.

37. Session portability and device substitution. Is there a mechanism whereby a component can proxy/stand-in for another when reloading old sessions? Can devices implementing this API exchange data with the other plug-in APIs in wider usage?

There is currently no extension for this.
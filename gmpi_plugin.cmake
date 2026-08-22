# settings that apply to every plugin format

# The directory this file lives in, captured while it is still being read.
# gmpi_version.rc.in sits beside it and gmpi_plugin() has to find it, but a
# function body cannot ask: CMAKE_CURRENT_LIST_DIR has dynamic scope, so inside
# a function it names the list file that CALLED it - the consumer's
# plugins/Foo/CMakeLists.txt - and not the file the function was defined in.
set(GMPI_PLUGIN_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

################################ plugin version #########################################
#
# A plugin's version is written in exactly ONE place: the `version` attribute of
# the <Plugin> element in its metadata XML.
#
#     <Plugin id="Acme: Gain" name="Gain" vendor="Acme" version="2.1.0">
#
# and from there it reaches everything that describes the plugin:
#
#   * the VST3 factory reports it to the host (MyVstPluginFactory::getClassInfo2)
#   * plist_util turns it into the AU component's version and CFBundleVersion
#   * the macOS bundle's CFBundleShortVersionString / CFBundleVersion, below
#   * a generated Windows VERSIONINFO resource, below
#
# The XML rather than the CMake project version, for a structural reason and not
# a stylistic one: MyVstPluginFactory.cpp is compiled ONCE into the shared
# VST3_Wrapper static library, so no per-plugin compile definition can ever
# reach it. A version that lives in CMake can only be handed to a runtime
# consumer as a define, and that route is closed. The XML, on the other hand, is
# already parsed per-plugin at load. So the version travels with the plugin's
# own description and CMake reads it from there - which costs a regex here, and
# nothing at runtime.
#
# What that costs: CMake cannot ask the SDK's parser, so it finds the attribute
# textually (gmpi_find_plugin_element), and the file it found it in becomes a
# configure dependency so that a bumped version cannot go stale.

# Locate the first <Plugin ...> opening tag in an ordered list of FILES, and
# return the tag text (out_tag) and the file it came from (out_file) so the
# caller can read its attributes. Both are empty when no file holds such a tag,
# which is not an error - it describes every plugin written before this existed.
# Files that do not exist are skipped, so a caller may name a candidate
# speculatively.
#
# The first tag found wins, so a module registering several plugins is described
# by its first one.
#
# This is a text search, not a parse, and the pattern is built to keep it out of
# trouble:
#
#   * whitespace is required directly after "Plugin", so <PluginList> cannot
#     match;
#   * [^>] cannot cross the end of a tag, so the <?xml version="1.0"?>
#     declaration and any `version=` on some other element cannot be picked up;
#   * the tag must BEGIN a line (only indentation before it), because prose
#     mentioning a tag does so mid-sentence. GMPI-plugins' own
#     Gain_with_resource_xml.cpp has "...must exactly match the XML's <Plugin
#     id="...">." in a comment, and without this it matched, beating the real
#     element in the .xml beside it.
#
# What remains: a <Plugin ...> tag written at the start of a line inside a
# comment is still indistinguishable from the real thing, because that is also
# where the real one is written. The cost of being fooled is a wrong name or
# version in a resource, never a broken build.
function(gmpi_find_plugin_element out_tag out_file)
    cmake_parse_arguments(ARG "" "" "FILES" ${ARGN})

    foreach(candidate IN LISTS ARG_FILES)
        if(NOT EXISTS "${candidate}" OR IS_DIRECTORY "${candidate}")
            continue()
        endif()

        file(READ "${candidate}" contents)
        string(REGEX MATCH "[\r\n][ \t]*(<Plugin[ \t\r\n][^>]*>)" tag "${contents}")
        set(tag "${CMAKE_MATCH_1}")

        if(NOT tag STREQUAL "")
            set(${out_tag} "${tag}" PARENT_SCOPE)
            set(${out_file} "${candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_tag} "" PARENT_SCOPE)
    set(${out_file} "" PARENT_SCOPE)
endfunction()

# One attribute out of an XML opening tag, or "" when the tag does not carry it.
# The leading whitespace class is what stops `version` matching the tail of a
# longer attribute name.
function(gmpi_xml_attribute out tag name)
    string(REGEX MATCH "[ \t\r\n]${name}[ \t\r\n]*=[ \t\r\n]*\"([^\"]*)\"" matched "${tag}")

    if(matched STREQUAL "")
        set(${out} "" PARENT_SCOPE)
    else()
        set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    endif()
endfunction()

# VERSIONINFO's FILEVERSION is four 16-bit integers, not a string, so reduce the
# declared version to at most four numbers and pad the rest with zeros. A
# version with a non-numeric tail ("2.1.0-beta") keeps its leading numbers here
# and its full text in the string fields beside them - which is how Windows
# treats its own.
#
# A version that begins with no digit at all ("v1.2", "beta") reduces to nothing,
# and 1,0,0,0 is substituted for the zero that would otherwise be written - the
# same substitution wrapper/common/plist_util.cpp makes for the AudioUnit's
# version integer, for the same reason: an installer or updater comparing
# FILEVERSION reads 0.0.0.0 as older than every build already shipped, so a
# plugin whose versions are all "v"-prefixed would publish one release after
# another that no comparison could tell apart. Which is the exact failure this
# resource exists to prevent, so it must not be reintroduced by the parse.
#
# The string fields are untouched and still say what the author wrote, so
# Explorer's Details tab shows "v1.2" beside a FILEVERSION of 1.0.0.0. A plugin
# that declares 0.0.0 outright is indistinguishable from an unparseable one here
# and gets the same treatment.
function(gmpi_version_quad out version)
    string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" numeric "${version}")
    string(REPLACE "." ";" parts "${numeric}")

    set(quad "")
    foreach(i RANGE 3)
        list(LENGTH parts count)
        if(i LESS count)
            list(GET parts ${i} part)
        else()
            set(part 0)
        endif()

        # "007" would be read as octal by the resource compiler.
        string(REGEX REPLACE "^0+([0-9])" "\\1" part "${part}")

        if(part GREATER 65535)
            set(part 65535)
        endif()

        list(APPEND quad "${part}")
    endforeach()

    list(JOIN quad "," quad_csv)

    if(quad_csv STREQUAL "0,0,0,0")
        set(quad_csv "1,0,0,0")
    endif()

    set(${out} "${quad_csv}" PARENT_SCOPE)
endfunction()

################################ plugin version #########################################

function(gmpi_target)
    set(options)
    set(oneValueArgs PROJECT_NAME)
    set(multiValueArgs)
    cmake_parse_arguments(GMPI_TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT GMPI_TARGET_PROJECT_NAME)
        message(FATAL_ERROR "gmpi_target(PROJECT_NAME <name>) is required.")
    endif()

    # Use target-based definitions and features
    target_compile_definitions(
        ${GMPI_TARGET_PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
    # Workspace uses C++14
    # target_compile_features(${GMPI_TARGET_PROJECT_NAME} PUBLIC cxx_std_17)

    if(APPLE)
        # These eight cache entries are filled in by gmpi_find_frameworks() in
        # the wrapper repo (wrapper/cmake/GmpiFrameworks.cmake), which each
        # wrapper calls; nothing in this SDK produces them, so their exact names
        # are a contract between the two repos.
        #
        # Each expands to nothing when no wrapper has been configured yet, which
        # includes the first configure of a project that add_subdirectory's the
        # wrappers AFTER its plugins - the ordering the note in gmpi_plugin()
        # below calls legal, and the one this tree uses. A plugin that links a
        # WRAPPER survives it: the wrappers link these frameworks with the plain
        # target_link_libraries signature, which puts them in the wrapper's link
        # INTERFACE too, so the module gets them regardless of what this call
        # saw. A standalone-only project has no such fallback - its plugin links
        # no wrapper at all - which is why Standalone/CMakeLists.txt finds every
        # framework named below, OpenGL included, whether it uses it or not.
        target_link_libraries(${GMPI_TARGET_PROJECT_NAME} PRIVATE
            ${COREFOUNDATION_LIBRARY}
            ${COCOA_LIBRARY}
            ${CORETEXT_LIBRARY}
            ${OPENGL_LIBRARY}
            ${IMAGEIO_LIBRARY}
            ${COREGRAPHICS_LIBRARY}
            ${ACCELERATE_LIBRARY}
            ${QUARTZCORE_LIBRARY}
        )
    endif()

    if(WIN32)
        # Prefer bare names; MSVC resolves .lib automatically
        target_link_libraries(${GMPI_TARGET_PROJECT_NAME} PRIVATE d3d11 d2d1 dwrite windowscodecs)
        target_link_options(${GMPI_TARGET_PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
    endif()
endfunction()

################################################################

function(gmpi_plugin)
    set(options HAS_DSP HAS_GUI HAS_XML IS_OFFICIAL_MODULE USE_STAGING)
    set(oneValueArgs PROJECT_NAME)
    set(multiValueArgs FORMATS_LIST SOURCE_FILES)
    cmake_parse_arguments(GMPI_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT GMPI_PLUGIN_PROJECT_NAME)
        message(FATAL_ERROR "gmpi_plugin(PROJECT_NAME <name>) is required.")
    endif()

    if(NOT GMPI_SDK)
        message(FATAL_ERROR "function(gmpi_plugin) requires GMPI_SDK to be set.")
    endif()
    if(NOT DEFINED GMPI_UI_SDK AND DEFINED HAS_GUI)
        message(FATAL_ERROR "function(gmpi_plugin) requires GMPI_UI_SDK to be set.")
    endif()
    
    # Default to GMPI if no formats were specified.
    if(NOT DEFINED GMPI_PLUGIN_FORMATS_LIST)
        set(GMPI_PLUGIN_FORMATS_LIST GMPI)
    endif()

    # never build AU on Windows
    if(NOT APPLE)
        list(REMOVE_ITEM GMPI_PLUGIN_FORMATS_LIST "AU")
    endif()

    # AU3 - the AUv3 app extension - is handled OUTSIDE the per-format loop
    # below: the loop's shape is one MODULE library per format, and an AUv3 is
    # two executables (the .appex and the containing app) plus an assembly
    # step. Note whether it was asked for, then take it off the list the loop
    # walks.
    list(FIND GMPI_PLUGIN_FORMATS_LIST "AU3" FIND_AU3_INDEX)
    list(REMOVE_ITEM GMPI_PLUGIN_FORMATS_LIST "AU3")
    if(NOT APPLE)
        set(FIND_AU3_INDEX -1)
    endif()

    # iOS has exactly one plugin format - the AUv3 extension - so on an iOS
    # configure every other format simply is not built there, the way AU is
    # not built on Windows. A plugin whose format list never mentions AU3
    # produces no targets at all on iOS, which is correct: there is nothing it
    # could ship.
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(GMPI_PLUGIN_FORMATS_LIST "")
        set(GMPI_AU3_IOS TRUE)
    else()
        set(GMPI_AU3_IOS FALSE)
    endif()

    # STANDALONE is an application, not a plugin: a window with a menu bar
    # wrapping one plugin, the way JUCE's standalone target does. Linux, Windows
    # and macOS all have a shell now; the guard stays because the format is
    # still dropped rather than fatal on anything else, which keeps a
    # cross-platform project's CMakeLists identical on every machine rather than
    # making each one guard the format itself.
    list(FIND GMPI_PLUGIN_FORMATS_LIST "STANDALONE" FIND_STANDALONE_INDEX)
    if(FIND_STANDALONE_INDEX GREATER_EQUAL 0 AND NOT (WIN32 OR UNIX))
        message(STATUS "gmpi_plugin(${GMPI_PLUGIN_PROJECT_NAME}): STANDALONE skipped (no shell on this platform yet)")
        list(REMOVE_ITEM GMPI_PLUGIN_FORMATS_LIST "STANDALONE")
    endif()

    # Standalone_Wrapper's Linux arm probes its dependencies and declines to
    # build when one is absent, so that a missing pipewire SDK costs the bare app
    # rather than the whole tree. Drop the format on the same machines: the
    # executable below links Standalone_Wrapper by name, and a name that never
    # becomes a target is only a raw linker flag as far as CMake is concerned --
    # the configure passes and the default build then fails, on exactly the
    # machines the wrapper's decline exists to protect.
    #
    # `if(TARGET Standalone_Wrapper)` cannot answer this. A plain name in
    # target_link_libraries is not resolved until generate time, so a project may
    # legally add_subdirectory() the wrappers AFTER the plugins that link them,
    # and every consumer in this tree does -- nothing obliges a third-party one
    # to order it the other way, so no wrapper target can be relied on to exist
    # at any point inside this function. Running the wrapper's own probe is the
    # only answer available this early, and it is the very file the wrapper
    # includes, so the two cannot disagree.
    list(FIND GMPI_PLUGIN_FORMATS_LIST "STANDALONE" FIND_STANDALONE_INDEX)
    if(FIND_STANDALONE_INDEX GREATER_EQUAL 0)
        if(NOT GMPI_ADAPTORS)
            message(FATAL_ERROR "function(gmpi_plugin) requires GMPI_ADAPTORS to be set.")
        endif()

        # OPTIONAL, over an explicitly emptied variable so nothing can leak in
        # from the caller's scope: this SDK and the wrappers are separate repos
        # and can be checked out at mismatched revisions. A wrapper tree that
        # predates the probe is one where nothing ever declined, so assume the
        # wrapper builds -- exactly the behaviour before this check existed --
        # rather than failing the configure over a file that on Windows and
        # macOS has nothing to say anyway.
        set(GMPI_STANDALONE_MISSING_DEPENDENCIES "")
        include("${GMPI_ADAPTORS}/wrapper/Standalone/dependencies.cmake" OPTIONAL)

        if(GMPI_STANDALONE_MISSING_DEPENDENCIES)
            list(JOIN GMPI_STANDALONE_MISSING_DEPENDENCIES ", " _standalone_missing_pretty)
            message(STATUS "gmpi_plugin(${GMPI_PLUGIN_PROJECT_NAME}): STANDALONE skipped -- Standalone_Wrapper cannot be built here, missing: ${_standalone_missing_pretty}")
            list(REMOVE_ITEM GMPI_PLUGIN_FORMATS_LIST "STANDALONE")
        endif()
    endif()

    #if building an AU or AU3, we're gonna need the GMPI also (for scanning the plist).
    # Except on iOS: an iOS-built module cannot be LOADED by the macOS machine
    # running the build, so there the plist comes from plist_util's --xml mode
    # reading the plugin's metadata straight from source, and no GMPI module is
    # needed (or buildable).
    list(FIND GMPI_PLUGIN_FORMATS_LIST "AU" FIND_AU_INDEX)
    if((FIND_AU_INDEX GREATER_EQUAL 0 OR FIND_AU3_INDEX GREATER_EQUAL 0) AND NOT GMPI_AU3_IOS)
        list(FIND GMPI_PLUGIN_FORMATS_LIST "GMPI" FIND_GMPI_INDEX)
        if(FIND_GMPI_INDEX LESS 0)
            list(APPEND GMPI_PLUGIN_FORMATS_LIST "GMPI")
        endif()

        # we're gonna need the corefoundation library for the plist util
        FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
        MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)
    endif()

################################ plist utility ##########################################
    if(FIND_AU3_INDEX GREATER_EQUAL 0 AND GMPI_AU3_IOS)
        # Cross-compiling: plist_util must RUN on this machine, so the iOS
        # toolchain in effect for every real target cannot build it. One raw
        # host-compiler invocation instead of a nested CMake project - the tool
        # is three sources and a framework.
        #
        # The path is set OUTSIDE the once-only guard: gmpi_plugin() is a
        # function, so the second plugin in a project enters here with fresh
        # variables but the target already made.
        set(GMPI_AU3_PLIST_UTIL_HOST "${CMAKE_BINARY_DIR}/plist_util_host")
        if(NOT TARGET plist_util_host)
            add_custom_command(
                OUTPUT "${GMPI_AU3_PLIST_UTIL_HOST}"
                COMMAND xcrun -sdk macosx clang++ -std=c++20
                    "${GMPI_ADAPTORS}/wrapper/common/plist_util.cpp"
                    "${GMPI_ADAPTORS}/wrapper/common/tinyXml2/tinyxml2.cpp"
                    "${GMPI_SDK}/Hosting/dynamic_linking.cpp"
                    -I "${GMPI_SDK}/Core"
                    -I "${GMPI_ADAPTORS}/wrapper/common"
                    -framework CoreFoundation
                    -o "${GMPI_AU3_PLIST_UTIL_HOST}"
                DEPENDS
                    "${GMPI_ADAPTORS}/wrapper/common/plist_util.cpp"
                    "${GMPI_ADAPTORS}/wrapper/common/tinyXml2/tinyxml2.cpp"
                    "${GMPI_SDK}/Hosting/dynamic_linking.cpp"
                COMMENT "Building plist_util for the build host"
                VERBATIM
            )
            add_custom_target(plist_util_host DEPENDS "${GMPI_AU3_PLIST_UTIL_HOST}")
        endif()
    endif()

    if(FIND_AU_INDEX GREATER_EQUAL 0 OR (FIND_AU3_INDEX GREATER_EQUAL 0 AND NOT GMPI_AU3_IOS))
        if(NOT TARGET plist_util) # ensure only built once if multiple AU plugins in same project
            # Spelled out rather than sharing the wrappers' GMPI_HOSTING_SRCS:
            # this is a different, smaller set - the XML reader and the dynamic
            # loader, none of the plugin-hosting layer - and gmpi_plugin() runs
            # in the CONSUMING project's scope, where a variable set inside the
            # wrapper repo's own directories does not exist.
            set(plist_srcs
                ${GMPI_SDK}/Hosting/xml_spec_reader.h
                ${GMPI_SDK}/Hosting/xml_spec_reader.cpp
                ${GMPI_SDK}/Hosting/dynamic_linking.h
                ${GMPI_SDK}/Hosting/dynamic_linking.cpp
                ${GMPI_ADAPTORS}/wrapper/common/plist_util.cpp
                ${GMPI_ADAPTORS}/wrapper/common/tinyXml2/tinyxml2.h
                ${GMPI_ADAPTORS}/wrapper/common/tinyXml2/tinyxml2.cpp
            )

            add_executable(plist_util ${plist_srcs})

            target_include_directories(plist_util PRIVATE
                ${GMPI_SDK}/Core
                ${GMPI_ADAPTORS}/wrapper/common
            )

            target_link_libraries( plist_util ${COREFOUNDATION_LIBRARY} )
        endif()
    endif()
################################ plist utility ##########################################

    # add SDK files
    set(plugin_includes
        ${GMPI_SDK}
        ${GMPI_SDK}/Core
    )
    
    set(sdk_srcs
        ${GMPI_SDK}/Core/Common.h
        ${GMPI_SDK}/Core/Common.cpp
        ${GMPI_SDK}/Core/RefCountMacros.h
        ${GMPI_SDK}/Core/GmpiApiCommon.h
        ${GMPI_SDK}/Core/GmpiSdkCommon.h
    )

    if(GMPI_PLUGIN_HAS_DSP)
        list(APPEND sdk_srcs
            ${GMPI_SDK}/Core/Processor.h
            ${GMPI_SDK}/Core/Processor.cpp
            ${GMPI_SDK}/Core/GmpiApiAudio.h
        )
    endif()

    if(GMPI_PLUGIN_HAS_GUI)
        list(APPEND sdk_srcs
            ${GMPI_UI_SDK}/GmpiApiDrawing.h
            ${GMPI_SDK}/Core/GmpiApiEditor.h
            ${GMPI_UI_SDK}/Drawing.h
        )
        list(APPEND plugin_includes ${GMPI_UI_SDK})
    endif()

    if(GMPI_PLUGIN_HAS_XML)
        set(resource_srcs
            ${GMPI_PLUGIN_PROJECT_NAME}.xml
        )

        if(WIN32)
            list(APPEND resource_srcs ${GMPI_PLUGIN_PROJECT_NAME}.rc)
            source_group(resources FILES ${resource_srcs})
        endif()
    endif()

    # How the plugin describes itself. Read once here; the macOS bundle keys and
    # the Windows VERSIONINFO resource below are the consumers. See the comment
    # block above gmpi_find_plugin_element() for why the XML is the source.
    #
    # Search order matters, and the rule is: only ever read the XML this build
    # actually uses.
    #
    #   * SOURCE_FILES first. A Register<>::withXml() plugin keeps its metadata
    #     as a raw string literal in its C++, and that string is what every
    #     wrapper's factory parses at load - so it is the plugin's real identity
    #     whatever else is lying around.
    #   * <PROJECT_NAME>.xml only under HAS_XML, which is the option that
    #     actually compiles it into the module. Without HAS_XML a file of that
    #     name is not part of the build at all, and trusting it is worse than
    #     ignoring it: GMPI-plugins/plugins/FreqAnalyser has a leftover one
    #     describing a plugin with a different id to the one the source
    #     registers, so reading it would have stamped a version onto a binary
    #     that reports something else at runtime - the exact drift this is here
    #     to close.
    set(plugin_xml_candidates "")
    foreach(src IN LISTS GMPI_PLUGIN_SOURCE_FILES)
        if(IS_ABSOLUTE "${src}")
            list(APPEND plugin_xml_candidates "${src}")
        else()
            list(APPEND plugin_xml_candidates "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        endif()
    endforeach()
    if(GMPI_PLUGIN_HAS_XML)
        list(APPEND plugin_xml_candidates "${CMAKE_CURRENT_SOURCE_DIR}/${GMPI_PLUGIN_PROJECT_NAME}.xml")
    endif()

    gmpi_find_plugin_element(plugin_xml_tag plugin_xml_file FILES ${plugin_xml_candidates})

    if(plugin_xml_tag STREQUAL "")
        # WARNING rather than STATUS, and the difference matters because this is
        # the one outcome here that produces a WRONG binary rather than a
        # defaulted one.
        #
        # A module with no <Plugin> element anywhere CMake looked still has one
        # somewhere - a plugin that really had none would register nothing and
        # load as empty - so the wrappers go on parsing it and reporting the
        # author's real version to the DAW, while the resource and the bundle
        # keys below say 1.0.0. Two answers to the same question, from one
        # build, which is the precise failure this whole mechanism exists to
        # remove. It is also silent: nothing at runtime looks wrong, the version
        # is simply wrong in the one place a user goes to read it off the file.
        #
        # STATUS would be the right level for an author who has merely not
        # opted in - that case is the `version` attribute being absent, a few
        # lines down, and says nothing at all. This one is the search failing,
        # which is not a choice anybody made. It fires on none of the SDK's own
        # sample plugins, so it is signal rather than a warning to learn to
        # ignore.
        message(WARNING
            "gmpi_plugin(${GMPI_PLUGIN_PROJECT_NAME}): no <Plugin> element was found in "
            "this plugin's sources, so the version, vendor and product name stamped on the "
            "built files are defaults (${GMPI_PLUGIN_PROJECT_NAME} / GMPI / 1.0.0) and will "
            "not match what the plugin reports to a host.\n"
            "Looked for a <Plugin ...> tag at the start of a line in each of SOURCE_FILES, "
            "and in ${GMPI_PLUGIN_PROJECT_NAME}.xml when HAS_XML is set. If the metadata "
            "lives somewhere else, add that file to SOURCE_FILES.")
    endif()

    gmpi_xml_attribute(plugin_version   "${plugin_xml_tag}" "version")
    gmpi_xml_attribute(plugin_vendor    "${plugin_xml_tag}" "vendor")
    gmpi_xml_attribute(plugin_nice_name "${plugin_xml_tag}" "name")

    if(plugin_version STREQUAL "")
        # The documented default: a plugin that declares no version still
        # builds, and is described exactly as every wrapper already described
        # it - the VST3 and CLAP factories both hardcoded "1.0.0" and the AU's
        # version integer was 65536, which is the same thing. (The macOS
        # standalone bundle said "1.0" here; it now says "1.0.0" like the rest,
        # which is the point of having one answer.)
        #
        # Kept in step with gmpi::hosting::defaultPluginVersion, which is in
        # this same repository (Hosting/xml_spec_reader.h).
        set(plugin_version "1.0.0")
    endif()
    if(plugin_vendor STREQUAL "")
        # Matches the fallback in Hosting/xml_spec_reader.cpp, so the vendor a
        # host is told and the vendor on the file's Details tab are the same.
        set(plugin_vendor "GMPI")
    endif()
    if(plugin_nice_name STREQUAL "")
        # Not the reader's fallback (it uses the plugin id, which CMake has no
        # reason to prefer) - just the target name, which is what an author
        # would recognise on a file's Details tab.
        set(plugin_nice_name "${GMPI_PLUGIN_PROJECT_NAME}")
    endif()

    # Whatever file the XML was found in is a configure input, C++ source
    # included. The version is read once, here, and baked into a bundle key and
    # a resource script; without this a bumped version would reach the plugin's
    # own factory - it is compiled in - while the bundle and the .exe went on
    # reporting the old one until something unrelated happened to re-run CMake.
    # Two releases indistinguishable on disk is the exact bug this whole
    # mechanism exists to fix, so it must not be reintroduced one layer down.
    #
    # The price is a re-configure whenever that file's timestamp moves, which
    # for a Register<>::withXml() plugin means every edit to the source holding
    # its XML. Nothing is rebuilt that would not have been rebuilt anyway; it
    # costs the configure step, which is why the file is registered only when
    # there was a <Plugin> element in it to read.
    if(NOT plugin_xml_file STREQUAL "")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${plugin_xml_file}")
    endif()

    foreach(kind IN LISTS GMPI_PLUGIN_FORMATS_LIST)
        if(kind STREQUAL "GMPI")
            set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME})
        else()
            set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_${kind})

            if(NOT GMPI_ADAPTORS)
                message(FATAL_ERROR "function(gmpi_plugin) requires GMPI_ADAPTORS to be set.")
            endif()
        endif()

        set(FORMAT_SDK_FILES ${sdk_srcs})

        if(kind STREQUAL "VST3")
            list(APPEND FORMAT_SDK_FILES ${GMPI_ADAPTORS}/wrapper/VST3/wrapperVst3.cpp)
            if(APPLE)
                # TODO wrap this into wrapperVst3.cpp ?
                list(APPEND FORMAT_SDK_FILES ${VST3_SDK}/public.sdk/source/main/macmain.cpp)
            endif()
        endif()
        
        if(kind STREQUAL "AU")
            list(APPEND FORMAT_SDK_FILES ${GMPI_ADAPTORS}/wrapper/AU2/wrapperAu2.cpp)
        endif()
        
        if(kind STREQUAL "CLAP")
            list(APPEND FORMAT_SDK_FILES ${GMPI_ADAPTORS}/wrapper/CLAP/wrapperClap.cpp)
        endif()

        if(kind STREQUAL "STANDALONE")
            # The entry point goes in the EXECUTABLE, not in Standalone_Wrapper:
            # a main() inside a static archive is never pulled in, because
            # nothing references it.
            if(WIN32)
                list(APPEND FORMAT_SDK_FILES ${GMPI_ADAPTORS}/wrapper/Standalone/windows/MainWin32.cpp)
            elseif(APPLE)
                list(APPEND FORMAT_SDK_FILES ${GMPI_ADAPTORS}/wrapper/Standalone/mac/MainMac.mm)
            else()
                list(APPEND FORMAT_SDK_FILES ${GMPI_ADAPTORS}/wrapper/Standalone/linux/MainWayland.cpp)
            endif()
        endif()

        # Organize SDK files in IDE
        source_group(sdk FILES ${FORMAT_SDK_FILES})

        # A VERSIONINFO resource, per target because the file name and the file
        # type differ per format. macOS and Linux carry this metadata in the
        # bundle or not at all; on Windows it has to be compiled in.
        set(version_srcs "")
        if(WIN32)
            # A plugin that ships its own .rc is left alone if that .rc already
            # declares a VERSIONINFO: a binary may hold only one, and the
            # author's is the more specific. Only HAS_XML compiles such a file,
            # and the .rc it names may well have been written by hand.
            set(author_rc_has_version FALSE)
            if(GMPI_PLUGIN_HAS_XML AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${GMPI_PLUGIN_PROJECT_NAME}.rc")
                file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${GMPI_PLUGIN_PROJECT_NAME}.rc" author_rc_text)
                string(FIND "${author_rc_text}" "VERSIONINFO" author_rc_version_pos)
                if(NOT author_rc_version_pos EQUAL -1)
                    set(author_rc_has_version TRUE)
                    message(STATUS "gmpi_plugin(${SUB_PROJECT_NAME}): keeping the VERSIONINFO in ${GMPI_PLUGIN_PROJECT_NAME}.rc, not generating one")
                endif()
            endif()

            if(NOT author_rc_has_version)
                if(kind STREQUAL "STANDALONE")
                    set(GMPI_RC_FILETYPE "VFT_APP")
                    set(GMPI_RC_ORIGINAL_FILENAME "${SUB_PROJECT_NAME}.exe")
                    set(GMPI_RC_DESCRIPTION "${plugin_nice_name} (standalone application)")
                else()
                    # AU never reaches here - it is removed from the format list
                    # on anything but macOS - so the lower-cased format name is
                    # the file extension, exactly as TARGET_EXTENSION works out
                    # below.
                    string(TOLOWER "${kind}" version_rc_extension)
                    set(GMPI_RC_FILETYPE "VFT_DLL")
                    set(GMPI_RC_ORIGINAL_FILENAME "${SUB_PROJECT_NAME}.${version_rc_extension}")
                    set(GMPI_RC_DESCRIPTION "${plugin_nice_name} (${kind} plug-in)")
                endif()

                set(GMPI_RC_VERSION "${plugin_version}")
                gmpi_version_quad(GMPI_RC_VERSION_QUAD "${plugin_version}")
                set(GMPI_RC_COMPANY "${plugin_vendor}")
                set(GMPI_RC_PRODUCT "${plugin_nice_name}")
                set(GMPI_RC_INTERNAL_NAME "${SUB_PROJECT_NAME}")

                if(plugin_xml_file STREQUAL "")
                    set(GMPI_RC_SOURCE "no <Plugin> element found - defaults used")
                else()
                    set(GMPI_RC_SOURCE "${plugin_xml_file}")
                endif()

                # Every value above is interpolated into a double-quoted string
                # in the resource script, so a quote inside one would end the
                # value early and leave a script that will not compile.
                foreach(rc_value COMPANY DESCRIPTION INTERNAL_NAME ORIGINAL_FILENAME PRODUCT SOURCE VERSION)
                    string(REPLACE "\"" "'" GMPI_RC_${rc_value} "${GMPI_RC_${rc_value}}")
                endforeach()

                set(version_srcs "${CMAKE_CURRENT_BINARY_DIR}/${SUB_PROJECT_NAME}.version.rc")
                configure_file("${GMPI_PLUGIN_CMAKE_DIR}/gmpi_version.rc.in" "${version_srcs}" @ONLY)
                source_group(resources FILES ${version_srcs})
            endif()
        endif()

        if(kind STREQUAL "STANDALONE")
            add_executable(${SUB_PROJECT_NAME}
                ${GMPI_PLUGIN_SOURCE_FILES}
                ${FORMAT_SDK_FILES}
                ${resource_srcs}
                ${version_srcs}
                ${wrapper_srcs}
            )
        else()
            add_library(${SUB_PROJECT_NAME} MODULE
                ${GMPI_PLUGIN_SOURCE_FILES}
                ${FORMAT_SDK_FILES}
                ${resource_srcs}
                ${version_srcs}
                ${wrapper_srcs}
            )
        endif()

        # Target-based includes
        if(plugin_includes)
            target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${plugin_includes})
        endif()
        # Adaptor headers
        if(GMPI_ADAPTORS)
            target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${GMPI_ADAPTORS})
        endif()

        gmpi_target(PROJECT_NAME ${SUB_PROJECT_NAME})
        
        #set(TARGET_EXTENSION "${kind}")
        string(TOLOWER "${kind}" TARGET_EXTENSION)

        if(kind STREQUAL "AU")
            set(TARGET_EXTENSION "component")

            # handle the Info.plist generation for the AU plugin
            # here is the plist output file
            set(PLIST_OUT "${CMAKE_CURRENT_BINARY_DIR}/${SUB_PROJECT_NAME}-Info.plist")

            # Xcode�s intermediate Info.plist path (what Build Settings shows) Cmake secretly copies the generated plist here.
            set(PLIST_DEST "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${SUB_PROJECT_NAME}.dir/Info.plist")

            # Ensure a stub plist exists at configure time (avoids "file not found" during project generation)
            #if(NOT EXISTS "${PLIST_DEST}")
            #    file(WRITE "${PLIST_DEST}" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\">\n<dict/>\n</plist>\n")
            #endif()

            # Drive the generation and make the AU target wait for it
            #add_custom_target(${SUB_PROJECT_NAME}_gen_plist DEPENDS "${PLIST_OUT}")

            # can't build the AU till we have the plist util and also the GMPI plugin (to scan it's XML)
            add_dependencies(${SUB_PROJECT_NAME} plist_util ${GMPI_PLUGIN_PROJECT_NAME}) # ensure plist util and GMPI plugin are built first.

            # this required me to run CMake, build the plugin, then re-run CMake to get the path correct.
#            # Generate Info.plist using plist_util by scanning the GMPI bundle
#            add_custom_command(TARGET ${SUB_PROJECT_NAME} PRE_LINK
#                COMMAND ${CMAKE_COMMAND} -E make_directory
#                        "$<SHELL_PATH:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${SUB_PROJECT_NAME}.dir>"
#                COMMAND $<TARGET_FILE:plist_util>
#                        "$<TARGET_BUNDLE_DIR:${GMPI_PLUGIN_PROJECT_NAME}>"  # scan input (GMPI bundle)
#                        "$<SHELL_PATH:${PLIST_DEST}>"                       # write where Xcode expects
#                # ARGS "$<TARGET_BUNDLE_DIR:${GMPI_PLUGIN_PROJECT_NAME}>" "${PLIST_OUT}"  # input bundle to scan, output plist
#                # DEPENDS plist_util  ${GMPI_PLUGIN_PROJECT_NAME}                         # build tool and GMPI first                                                     
#                # BYPRODUCTS "${PLIST_OUT}"
#                COMMENT "Generating Info.plist for AU plugin"
#                VERBATIM
#            )
#            # Tell Xcode/Bundle step to use the generated plist
#            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
#                MACOSX_BUNDLE_INFO_PLIST "${PLIST_OUT}"
#                BUNDLE TRUE
#                BUNDLE_EXTENSION "component"
#            )

            # After the AU bundle is built, overwrite its Info.plist with one generated by scanning the GMPI plugin bundle.

            # Path to the AU component bundle
            set(AU_BUNDLE "$<TARGET_BUNDLE_DIR:${SUB_PROJECT_NAME}>")
            # Path to the Info.plist inside the AU bundle
            set(AU_PLIST "${AU_BUNDLE}/Contents/Info.plist")

            add_custom_command(TARGET ${SUB_PROJECT_NAME}
                POST_BUILD
                COMMAND $<TARGET_FILE:plist_util>
                    "$<TARGET_BUNDLE_DIR:${GMPI_PLUGIN_PROJECT_NAME}>"      # input: GMPI bundle to scan
                    "${AU_PLIST}"                                           # output: Info.plist to overwrite
                    # TELL it the executable name instead of letting it guess.
                    # plist_util defaults CFBundleExecutable to "<stem>_AU",
                    # which is right only while the plugin leaves OUTPUT_NAME
                    # unset. When it is set the plist names a binary that is
                    # not there, and macOS refuses to register the component
                    # with "didn't find the component" -- a build that
                    # succeeds and a plug-in no host can load. Same defect as
                    # the Linux VST3 bundle name (TideSynth issue #271): a
                    # DERIVED name sitting beside a generator-expression one.
                    --exe-name "$<TARGET_FILE_BASE_NAME:${SUB_PROJECT_NAME}>"
                COMMENT "Overwriting Info.plist in AU component with plist_util"
                VERBATIM
            )

        endif()
            
        if(kind STREQUAL "STANDALONE")
            # An application, so no plugin extension - just Foo_STANDALONE,
            # which on Windows and Linux is also what you type to run it.
            #
            # ".exe" on Windows, and not because it is prettier: a file without
            # it is not executable at all there, so the blanket SUFFIX "" that
            # is right everywhere else would produce a target that builds and
            # then cannot be run.
            if(WIN32)
                set_target_properties(${SUB_PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".exe")
            else()
                set_target_properties(${SUB_PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX "")
            endif()

            if(APPLE)
                # A .app, unlike the bare binary the other two platforms get,
                # and the reason is not tidiness:
                #
                #  * an app that opens an audio INPUT needs
                #    NSMicrophoneUsageDescription in an Info.plist, or macOS
                #    refuses the device. A bare Mach-O has no Info.plist, so the
                #    permission is attributed to whatever launched it - the
                #    Terminal - which is both confusing and unreliable.
                #  * NSHighResolutionCapable is what stops AppKit handing the
                #    editor a 1x backing store on a Retina display and scaling
                #    it up.
                #
                # The binary is still directly runnable, at
                # Foo_STANDALONE.app/Contents/MacOS/Foo_STANDALONE, which is the
                # path a test harness or the MCP server should launch.
                # The version the author declared, verbatim, in both keys.
                # Apple wants period-separated integers in these, so a version
                # with a tail ("2.1.0-beta") will be rejected by App Store
                # validation - it is fine everywhere else, and reporting
                # something the author did not write would be worse.
                set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
                    MACOSX_BUNDLE TRUE
                    MACOSX_BUNDLE_INFO_PLIST "${GMPI_ADAPTORS}/wrapper/Standalone/mac/Info.plist.in"
                    MACOSX_BUNDLE_BUNDLE_NAME "${GMPI_PLUGIN_PROJECT_NAME}"
                    MACOSX_BUNDLE_GUI_IDENTIFIER "com.gmpi.standalone.${GMPI_PLUGIN_PROJECT_NAME}"
                    MACOSX_BUNDLE_BUNDLE_VERSION "${plugin_version}"
                    MACOSX_BUNDLE_SHORT_VERSION_STRING "${plugin_version}"
                )
            endif()
        elseif(APPLE)
            # The version keys reach CMake's own bundle Info.plist template,
            # which is what the .vst3, .clap and .gmpi bundles get. The .component
            # does not keep them: plist_util overwrites the AU's Info.plist
            # wholesale after the build, and writes the same version there from
            # the same XML attribute.
            set_target_properties(${SUB_PROJECT_NAME}
            PROPERTIES
                BUNDLE TRUE
                BUNDLE_EXTENSION "${TARGET_EXTENSION}"
                MACOSX_BUNDLE_BUNDLE_VERSION "${plugin_version}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "${plugin_version}"
        )
        else()
            # PREFIX "" so Linux does not produce libFoo.gmpi where Windows and
            # macOS produce Foo.gmpi. No-op on Windows, where the shared-module
            # prefix is already empty.
            set_target_properties(${SUB_PROJECT_NAME}
                PROPERTIES
                PREFIX ""
                SUFFIX ".${TARGET_EXTENSION}"
            )
        endif()

        if(UNIX AND NOT APPLE AND GMPI_PLUGIN_HAS_XML)
            # Linux: no resource embedding or bundle; place the XML beside the
            # binary (<binary>.xml) where the module scanner looks for it.
            add_custom_command(TARGET ${SUB_PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/${GMPI_PLUGIN_PROJECT_NAME}.xml"
                    "$<TARGET_FILE:${SUB_PROJECT_NAME}>.xml"
                VERBATIM)
        endif()
    endforeach()

    list(FIND GMPI_PLUGIN_FORMATS_LIST "GMPI" FIND_GMPI_INDEX)
    list(FIND GMPI_PLUGIN_FORMATS_LIST "VST3" FIND_VST3_INDEX)
    list(FIND GMPI_PLUGIN_FORMATS_LIST "AU" FIND_AU_INDEX)
    list(FIND GMPI_PLUGIN_FORMATS_LIST "CLAP" FIND_CLAP_INDEX)
    list(FIND GMPI_PLUGIN_FORMATS_LIST "STANDALONE" FIND_STANDALONE_INDEX)

    if(FIND_STANDALONE_INDEX GREATER_EQUAL 0)
        set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_STANDALONE)

        # PUBLIC on the wrapper's side, so the generated Wayland protocol
        # headers and the gmpi_ui include paths reach MainWayland.cpp, which
        # this executable compiles itself.
        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE Standalone_Wrapper)

        set_target_properties(${SUB_PROJECT_NAME} PROPERTIES FOLDER "standalone apps")
    endif()

    if(FIND_VST3_INDEX GREATER_EQUAL 0)
        set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_VST3)

        target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${VST3_SDK})
        
        if(APPLE)
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "vst3")

            if(GMPI_PLUGIN_HAS_XML)
                # Place xml file in bundle 'Resources' folder.
                set(xml_path "${SUB_PROJECT_NAME}.xml")
                target_sources(${SUB_PROJECT_NAME} PUBLIC ${xml_path})
                set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            endif()
        elseif(UNIX)
            # Linux VST3s are bundle DIRECTORIES, not bare shared objects - a host
            # scanning ~/.vst3 looks for "<name>.vst3/Contents/<arch>-linux/<name>.so"
            # and ignores a loose .so entirely. Build a plain .so, then assemble the
            # bundle around it after linking.
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".so")

            # The bundle DIRECTORY must match the .so inside it -- a host looks for
            # "<name>.vst3/Contents/<arch>-linux/<name>.so". $<TARGET_FILE_BASE_NAME>
            # follows OUTPUT_NAME, which is what names the .so; ${SUB_PROJECT_NAME} is
            # the TARGET, which does not. They diverge the moment a plugin sets
            # OUTPUT_NAME, and the result loads on macOS and Windows (whose bundle
            # names come from OUTPUT_NAME for free) while silently failing here.
            set(vst3_bundle "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${SUB_PROJECT_NAME}>.vst3")
            set(vst3_bundle_arch "${vst3_bundle}/Contents/${CMAKE_SYSTEM_PROCESSOR}-linux")

            add_custom_command(TARGET ${SUB_PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${vst3_bundle_arch}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${SUB_PROJECT_NAME}>" "${vst3_bundle_arch}/"
                COMMENT "Assembling VST3 bundle $<TARGET_FILE_BASE_NAME:${SUB_PROJECT_NAME}>.vst3"
                VERBATIM
            )

            if(GMPI_PLUGIN_HAS_XML)
                add_custom_command(TARGET ${SUB_PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${vst3_bundle}/Contents/Resources"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${CMAKE_CURRENT_SOURCE_DIR}/${SUB_PROJECT_NAME}.xml"
                        "${vst3_bundle}/Contents/Resources/"
                    VERBATIM
                )
            endif()
        else()
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES SUFFIX ".vst3")
        endif()

        # Link the VST3 wrapper as static libs
        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE VST3_Wrapper)
    endif()

    if(FIND_AU_INDEX GREATER_EQUAL 0)
        set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_AU)
        
        # Link the AU2 wrapper as static libs
        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE AU_Wrapper)
        target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${AU_SDK_H})

        # copy plugin to components folder. NOTE: Requires user to have read-write permissions on folder.
        if(SE_LOCAL_BUILD)
            SET(AU_DEST "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
            add_custom_command(TARGET ${SUB_PROJECT_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "$<TARGET_BUNDLE_DIR:${SUB_PROJECT_NAME}>"
                    "${AU_DEST}/$<TARGET_FILE_NAME:${SUB_PROJECT_NAME}>.component"
                COMMENT "Copy to AU folder"
                VERBATIM
            )
        endif()
    endif()

    if(FIND_AU3_INDEX GREATER_EQUAL 0)
        # The AUv3 app extension: <Plugin>_AU3.appex inside <Plugin>_AU3App.app.
        # Two executables and an assembly step, which is why this lives outside
        # the MODULE-per-format loop above. One block serves macOS and iOS;
        # $<TARGET_BUNDLE_CONTENT_DIR> is what absorbs the layouts (.app/Contents
        # on macOS, the flat .app on iOS).
        set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_AU3)
        set(AU3_APP_NAME ${GMPI_PLUGIN_PROJECT_NAME}_AU3App)

        if(GMPI_AU3_IOS AND plugin_xml_file STREQUAL "")
            # On iOS the appex plist can only come from the plugin's declared
            # metadata (no loadable module to scan), and the search above found
            # none - which on macOS merely mis-stamps a version, but here means
            # an extension with no identity at all.
            message(FATAL_ERROR
                "gmpi_plugin(${GMPI_PLUGIN_PROJECT_NAME}): AU3 on iOS needs the plugin's "
                "<Plugin> metadata XML, and none was found in SOURCE_FILES or "
                "${GMPI_PLUGIN_PROJECT_NAME}.xml. Add the file holding it to SOURCE_FILES.")
        endif()

        # ---- the .appex ----
        # The same plugin sources every other format target compiles (so
        # MP_GetFactory resolves), plus the wrapper's force-link entry - the
        # extension's principal class is looked up by NAME from the Info.plist,
        # so without wrapperAu3.mm the linker strips it out of the static
        # library. See that file.
        add_executable(${SUB_PROJECT_NAME} MACOSX_BUNDLE
            ${GMPI_PLUGIN_SOURCE_FILES}
            ${sdk_srcs}
            ${resource_srcs}
            ${GMPI_ADAPTORS}/wrapper/AU3/wrapperAu3.mm
        )
        source_group(sdk FILES ${sdk_srcs})

        if(plugin_includes)
            target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${plugin_includes})
        endif()
        target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${GMPI_ADAPTORS})

        if(GMPI_AU3_IOS)
            # Not gmpi_target(): its APPLE arm links Cocoa and OpenGL, which do
            # not exist on iOS. The frameworks the appex really needs arrive
            # through AU3_Wrapper's link interface; only the config defines are
            # wanted here.
            target_compile_definitions(${SUB_PROJECT_NAME} PRIVATE
                $<$<CONFIG:Debug>:_DEBUG>
                $<$<CONFIG:Release>:NDEBUG>
            )
        else()
            gmpi_target(PROJECT_NAME ${SUB_PROJECT_NAME})
        endif()

        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE AU3_Wrapper)

        # App-extension entry point; Foundation provides it.
        target_link_options(${SUB_PROJECT_NAME} PRIVATE "-e" "_NSExtensionMain")

        if(GMPI_PLUGIN_HAS_XML)
            set_source_files_properties(${GMPI_PLUGIN_PROJECT_NAME}.xml PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endif()

        # The identifier must be PREFIXED by the containing app's - that pair
        # is how the system ties extension to app.
        set(AU3_APP_BUNDLE_ID "com.gmpi.au3.${GMPI_PLUGIN_PROJECT_NAME}")
        set(AU3_APPEX_BUNDLE_ID "${AU3_APP_BUNDLE_ID}.extension")

        # CMake's default bundle plist is only a stub to link against; the real
        # one - NSExtension, AudioComponents, the identity fourCCs - is written
        # over it by plist_util. On macOS that scans the built GMPI module,
        # exactly as the AU2 .component's plist is made; on iOS the module
        # cannot be loaded by the machine doing the building, so the HOST-built
        # plist_util reads the same metadata from the plugin's own declaration
        # (--xml). One derivation either way, so the v2 and v3 releases of a
        # plugin cannot disagree about who they are.
        set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
            BUNDLE_EXTENSION "appex"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${AU3_APPEX_BUNDLE_ID}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${plugin_version}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${plugin_version}"
        )

        if(GMPI_AU3_IOS)
            add_dependencies(${SUB_PROJECT_NAME} plist_util_host)

            add_custom_command(TARGET ${SUB_PROJECT_NAME}
                POST_BUILD
                COMMAND "${GMPI_AU3_PLIST_UTIL_HOST}" --xml
                    "${plugin_xml_file}"                                        # input: the plugin's metadata declaration
                    "$<TARGET_BUNDLE_CONTENT_DIR:${SUB_PROJECT_NAME}>/Info.plist" # output: Info.plist to overwrite
                    --au3 "${SUB_PROJECT_NAME}" "${AU3_APPEX_BUNDLE_ID}"
                COMMENT "Overwriting Info.plist in AU3 appex with plist_util (--xml)"
                VERBATIM
            )
        else()
            add_dependencies(${SUB_PROJECT_NAME} plist_util ${GMPI_PLUGIN_PROJECT_NAME})

            add_custom_command(TARGET ${SUB_PROJECT_NAME}
                POST_BUILD
                COMMAND $<TARGET_FILE:plist_util>
                    "$<TARGET_BUNDLE_DIR:${GMPI_PLUGIN_PROJECT_NAME}>"          # input: GMPI bundle to scan
                    "$<TARGET_BUNDLE_CONTENT_DIR:${SUB_PROJECT_NAME}>/Info.plist" # output: Info.plist to overwrite
                    --au3 "${SUB_PROJECT_NAME}" "${AU3_APPEX_BUNDLE_ID}"
                COMMENT "Overwriting Info.plist in AU3 appex with plist_util"
                VERBATIM
            )
        endif()

        # ---- the containing app ----
        # An .appex cannot exist on its own; this app's one job is carrying it
        # in PlugIns/, where the system finds and registers it.
        #
        # Frameworks by raw flag, not ${COCOA_LIBRARY}: those cache entries are
        # filled by the WRAPPERS' find_library calls, and a consumer may legally
        # add_subdirectory the wrappers after its plugins - on a fresh configure
        # this app would then link no frameworks at all. The appex above is safe
        # either way (AU3_Wrapper's own interface carries its frameworks); this
        # app links no wrapper, so it must not lean on that cache.
        if(GMPI_AU3_IOS)
            add_executable(${AU3_APP_NAME} MACOSX_BUNDLE
                ${GMPI_ADAPTORS}/wrapper/AU3/ios/HostAppMain.mm
            )
            target_link_libraries(${AU3_APP_NAME} PRIVATE "-framework UIKit" "-framework Foundation")
            set(AU3_APP_PLIST "${GMPI_ADAPTORS}/wrapper/AU3/ios/HostApp-Info.plist.in")
        else()
            add_executable(${AU3_APP_NAME} MACOSX_BUNDLE
                ${GMPI_ADAPTORS}/wrapper/AU3/mac/HostAppMain.mm
            )
            target_link_libraries(${AU3_APP_NAME} PRIVATE "-framework Cocoa")
            set(AU3_APP_PLIST "${GMPI_ADAPTORS}/wrapper/AU3/mac/HostApp-Info.plist.in")
        endif()

        set_target_properties(${AU3_APP_NAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${AU3_APP_PLIST}"
            MACOSX_BUNDLE_BUNDLE_NAME "${GMPI_PLUGIN_PROJECT_NAME}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${AU3_APP_BUNDLE_ID}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${plugin_version}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${plugin_version}"
        )

        # ---- assembly ----
        # An always-run custom target, NOT a POST_BUILD on the app: a change to
        # the appex alone leaves the app up-to-date, and its POST_BUILD would
        # not re-run - deploying a stale extension that looks exactly like
        # "my change did nothing". Signed inside-out, ad-hoc.
        set(AU3_ASSEMBLE_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_CONTENT_DIR:${AU3_APP_NAME}>/PlugIns"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "$<TARGET_BUNDLE_DIR:${SUB_PROJECT_NAME}>"
                "$<TARGET_BUNDLE_CONTENT_DIR:${AU3_APP_NAME}>/PlugIns/${SUB_PROJECT_NAME}.appex"
        )

        if(GMPI_AU3_IOS)
            # No entitlements file: iOS processes are sandboxed by definition,
            # and com.apple.security.app-sandbox is a macOS key. The ad-hoc
            # signature satisfies the simulator; a device build re-signs with a
            # real identity outside this build.
            list(APPEND AU3_ASSEMBLE_COMMANDS
                COMMAND codesign --force --sign -
                    "$<TARGET_BUNDLE_CONTENT_DIR:${AU3_APP_NAME}>/PlugIns/${SUB_PROJECT_NAME}.appex"
                COMMAND codesign --force --sign -
                    "$<TARGET_BUNDLE_DIR:${AU3_APP_NAME}>"
            )
        else()
            # The sandbox entitlement is the one thing a macOS extension cannot
            # load without.
            list(APPEND AU3_ASSEMBLE_COMMANDS
                COMMAND codesign --force --sign - --entitlements "${GMPI_ADAPTORS}/wrapper/AU3/appex.entitlements"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${AU3_APP_NAME}>/PlugIns/${SUB_PROJECT_NAME}.appex"
                COMMAND codesign --force --sign -
                    "$<TARGET_BUNDLE_DIR:${AU3_APP_NAME}>"
            )
        endif()

        # Install and register for local development, INSIDE the assemble
        # target so it always runs after a fresh appex is in place - the
        # copy_plugin() route the other formats use is a POST_BUILD on their
        # one target, and here that ordering trap is the whole reason assemble
        # exists. macOS only: on iOS "install" means a simulator or device -
        # `xcrun simctl install booted <app>` after building, and launching it
        # once registers the extension there.
        if(SE_LOCAL_BUILD AND NOT GMPI_AU3_IOS)
            list(APPEND AU3_ASSEMBLE_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "$<TARGET_BUNDLE_DIR:${AU3_APP_NAME}>"
                    "$ENV{HOME}/Applications/${AU3_APP_NAME}.app"
                COMMAND pluginkit -a
                    "$ENV{HOME}/Applications/${AU3_APP_NAME}.app/Contents/PlugIns/${SUB_PROJECT_NAME}.appex"
            )
        endif()

        add_custom_target(${SUB_PROJECT_NAME}_assemble ALL
            ${AU3_ASSEMBLE_COMMANDS}
            COMMENT "Assembling ${AU3_APP_NAME}.app with ${SUB_PROJECT_NAME}.appex"
            VERBATIM
        )
        add_dependencies(${SUB_PROJECT_NAME}_assemble ${SUB_PROJECT_NAME} ${AU3_APP_NAME})

        set_target_properties(${SUB_PROJECT_NAME} PROPERTIES FOLDER "AU3 plugins")
        set_target_properties(${AU3_APP_NAME} PROPERTIES FOLDER "AU3 plugins")
        set_target_properties(${SUB_PROJECT_NAME}_assemble PROPERTIES FOLDER "AU3 plugins")
    endif()

    if(FIND_CLAP_INDEX GREATER_EQUAL 0)
        set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_CLAP)
        
        # Link the AU2 wrapper as static libs
        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE CLAP_Wrapper)
        target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${CLAP_SDK}/include ${CLAP_HELPERS_SDK}/include)

        if(APPLE)
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "clap")

            if(GMPI_PLUGIN_HAS_XML)
                # Place xml file in bundle 'Resources' folder.
                set(xml_path "${SUB_PROJECT_NAME}.xml")
                target_sources(${SUB_PROJECT_NAME} PUBLIC ${xml_path})
                set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            endif()
        else()
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".clap")
        endif()

        # copy plugin to CLAP folder. NOTE: Requires user to have read-write permissions on folder.
        if(APPLE AND SE_LOCAL_BUILD)
            SET(CLAP_DEST "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP")
            add_custom_command(TARGET ${SUB_PROJECT_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "$<TARGET_BUNDLE_DIR:${SUB_PROJECT_NAME}>"
                    "${CLAP_DEST}/$<TARGET_FILE_NAME:${SUB_PROJECT_NAME}>.clap"
                COMMENT "Copy to CLAP folder"
                VERBATIM
            )
        endif()
    endif()

    if(SE_LOCAL_BUILD)
        function(copy_plugin TARGET_NAME DEST_DIR EXTENSION)
            if(WIN32)
                add_custom_command(TARGET ${TARGET_NAME}
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
                    COMMAND copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "${DEST_DIR}\\$(TargetName)$(TargetExt)"
                    COMMENT "Copy to ${DEST_DIR} folder"
                    VERBATIM
                )
            elseif(APPLE)
                add_custom_command(TARGET ${TARGET_NAME}
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>" "${DEST_DIR}/$<TARGET_FILE_NAME:${TARGET_NAME}>.${EXTENSION}"
                    COMMENT "Copy to ${DEST_DIR} folder"
                    VERBATIM
                )
            elseif(UNIX)
                # Only VST3 is a bundle DIRECTORY on Linux (assembled above);
                # .clap and .gmpi are plain shared objects. $<TARGET_BUNDLE_DIR>
                # is macOS-only, so the VST3 source path is spelled out.
                if(EXTENSION STREQUAL "vst3")
                    add_custom_command(TARGET ${TARGET_NAME}
                        POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                            "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${TARGET_NAME}>.vst3"
                            "${DEST_DIR}/$<TARGET_FILE_BASE_NAME:${TARGET_NAME}>.vst3"
                        COMMENT "Copy to ${DEST_DIR}"
                        VERBATIM
                    )
                else()
                    add_custom_command(TARGET ${TARGET_NAME}
                        POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "$<TARGET_FILE:${TARGET_NAME}>" "${DEST_DIR}/"
                        COMMENT "Copy to ${DEST_DIR}"
                        VERBATIM
                    )
                endif()
            endif()
        endfunction()
        
        if(WIN32)
            if(FIND_VST3_INDEX GREATER_EQUAL 0)
                copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}_VST3
                            "C:\\Program Files\\Common Files\\VST3" "vst3")
                set_target_properties(${GMPI_PLUGIN_PROJECT_NAME}_VST3 PROPERTIES FOLDER "VST3 plugins")
            endif()

            if(FIND_GMPI_INDEX GREATER_EQUAL 0)
                if(GMPI_PLUGIN_USE_STAGING)
                    # Debug: stage to modules-staged; Release: fall back to normal modules folder
                    copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}
                                "$<IF:$<CONFIG:Debug>,C:\\Program Files\\Common Files\\SynthEdit\\modules-staged,C:\\Program Files\\Common Files\\SynthEdit\\modules>" "gmpi")
                elseif(GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                    copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}
                                "C:\\SE\\SE16\\SynthEdit2\\PlugIns\\$(TargetName)$(TargetExt)\\Contents\\x86_64-win" "gmpi")
                else()
                    copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}
                                "C:\\Program Files\\Common Files\\SynthEdit\\modules" "gmpi")
                endif()
            endif()

            if(FIND_CLAP_INDEX GREATER_EQUAL 0)
                copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}_CLAP
                            "C:\\Program Files\\Common Files\\CLAP" "clap")
                set_target_properties(${GMPI_PLUGIN_PROJECT_NAME}_CLAP PROPERTIES FOLDER "CLAP plugins")
            endif()
        endif()

        if(APPLE)
            if(FIND_VST3_INDEX GREATER_EQUAL 0 AND NOT GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}_VST3
                            "$ENV{HOME}/Library/Audio/Plug-Ins/VST3" "vst3")
            endif()

            if(FIND_GMPI_INDEX GREATER_EQUAL 0 AND NOT GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                if(GMPI_PLUGIN_USE_STAGING)
                    # Debug: stage to GMPI-staged; Release: fall back to normal GMPI folder
                    copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}
                                "$<IF:$<CONFIG:Debug>,$ENV{HOME}/Library/Audio/Plug-Ins/GMPI-staged,$ENV{HOME}/Library/Audio/Plug-Ins/GMPI>" "gmpi")
                else()
                    copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}
                                "$ENV{HOME}/Library/Audio/Plug-Ins/GMPI" "gmpi")
                endif()
            endif()

            if(FIND_CLAP_INDEX GREATER_EQUAL 0 AND NOT GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}_CLAP
                            "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP" "clap")
            endif()
        endif()

        if(UNIX AND NOT APPLE)
            # The per-user locations every Linux host scans. Without this the
            # bundles only ever exist in the build tree, and a DAW quite
            # correctly never finds them - which looks exactly like a plugin
            # that fails to load.
            if(FIND_VST3_INDEX GREATER_EQUAL 0 AND NOT GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}_VST3 "$ENV{HOME}/.vst3" "vst3")
            endif()

            if(FIND_CLAP_INDEX GREATER_EQUAL 0 AND NOT GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}_CLAP "$ENV{HOME}/.clap" "clap")
            endif()

            # SynthEdit's own third-party module folder - see
            # SynthEditWayland/linux-package/README.md. XDG_DATA_HOME is not
            # consulted here because the scanner does not either.
            if(FIND_GMPI_INDEX GREATER_EQUAL 0 AND NOT GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                copy_plugin(${GMPI_PLUGIN_PROJECT_NAME}
                            "$ENV{HOME}/.local/share/SynthEdit/modules" "gmpi")

                # The sidecar goes too. On Windows and macOS the XML is embedded
                # as a resource; on Linux it is a file beside the binary, so
                # installing the binary alone leaves a module the scanner cannot
                # read its metadata from.
                if(GMPI_PLUGIN_HAS_XML)
                    add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "$<TARGET_FILE:${GMPI_PLUGIN_PROJECT_NAME}>.xml"
                            "$ENV{HOME}/.local/share/SynthEdit/modules/"
                        VERBATIM)
                endif()
            endif()
        endif()
    endif()

    # Group modules under solution folders
    if(FIND_GMPI_INDEX GREATER_EQUAL 0)
        set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES FOLDER "GMPI plugins")
    endif()
endfunction()

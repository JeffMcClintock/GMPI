# settings that apply to every plugin format

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
    target_compile_features(${GMPI_TARGET_PROJECT_NAME} PUBLIC cxx_std_17)

    if(APPLE)
        # Guard Apple-specific frameworks
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
    set(options HAS_DSP HAS_GUI HAS_XML IS_OFFICIAL_MODULE)
    set(oneValueArgs PROJECT_NAME)
    set(multiValueArgs FORMATS_LIST SOURCE_FILES)
    cmake_parse_arguments(GMPI_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT GMPI_PLUGIN_PROJECT_NAME)
        message(FATAL_ERROR "gmpi_plugin(PROJECT_NAME <name>) is required.")
    endif()

    # Default to GMPI if no formats were specified.
    if(NOT GMPI_PLUGIN_FORMATS_LIST)
        set(GMPI_PLUGIN_FORMATS_LIST GMPI)
    endif()

    # never build AU on Windows
    if(WIN32)
        list(REMOVE_ITEM GMPI_PLUGIN_FORMATS_LIST "AU")
    endif()

    #if building an AU, we're gonna need the GMPI also (for scanning the plist)
    list(FIND GMPI_PLUGIN_FORMATS_LIST "AU" FIND_AU_INDEX)
    if(FIND_AU_INDEX GREATER_EQUAL 0)
        list(FIND GMPI_PLUGIN_FORMATS_LIST "GMPI" FIND_GMPI_INDEX)
        if(FIND_GMPI_INDEX LESS 0)
            list(APPEND GMPI_PLUGIN_FORMATS_LIST "GMPI")
        endif()
    endif()

################################ plist utility ##########################################
    if(FIND_AU_INDEX GREATER_EQUAL 0)
        if(NOT TARGET plist_util)
            set(plist_srcs
                ${GMPI_ADAPTORS}/wrapper/common/plist_util.cpp
                ${GMPI_ADAPTORS}/wrapper/common/dynamic_linking.h
                ${GMPI_ADAPTORS}/wrapper/common/dynamic_linking.cpp
                ${GMPI_ADAPTORS}/wrapper/common/tinyXml2/tinyxml2.h
                ${GMPI_ADAPTORS}/wrapper/common/tinyXml2/tinyxml2.cpp
            )

            add_executable(plist_util ${plist_srcs})

            target_include_directories(plist_util PRIVATE
                ${gmpi_sdk_folder}/Core
                ${GMPI_ADAPTORS}/wrapper/common
            )

            target_link_libraries( plist_util ${COREFOUNDATION_LIBRARY} )
        endif()
    endif()
################################ plist utility ##########################################

    # add SDK files
    set(sdk_srcs
        ${gmpi_sdk_folder}/Core/Common.h
        ${gmpi_sdk_folder}/Core/Common.cpp
    )

    set(plugin_includes
        ${gmpi_sdk_folder}
        ${gmpi_sdk_folder}/Core
    )
   
    if(GMPI_PLUGIN_HAS_DSP)
        list(APPEND sdk_srcs
            ${gmpi_sdk_folder}/Core/Processor.h
            ${gmpi_sdk_folder}/Core/Processor.cpp
        )
    endif()

    if(GMPI_PLUGIN_HAS_GUI)
        list(APPEND sdk_srcs
            ${gmpi_ui_folder}/GmpiApiDrawing.h
            ${gmpi_ui_folder}/Drawing.h
        )
        list(APPEND plugin_includes ${gmpi_ui_folder})
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

    foreach(kind IN LISTS GMPI_PLUGIN_FORMATS_LIST)
        if(kind STREQUAL "GMPI")
            set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME})
        else()
            set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_${kind})
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

            # handle the Info.plist generation for the AU plugin
            # here is the plist output file
            set(PLIST_OUT "${CMAKE_CURRENT_BINARY_DIR}/${SUB_PROJECT_NAME}-Info.plist")

            # Ensure a stub plist exists at configure time (avoids "file not found" during project generation)
            if(NOT EXISTS "${PLIST_OUT}")
                file(WRITE "${PLIST_OUT}" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\">\n<dict/>\n</plist>\n")
            endif()

            # Drive the generation and make the AU target wait for it
            add_custom_target(${SUB_PROJECT_NAME}_gen_plist DEPENDS "${PLIST_OUT}")
        endif()
        
        # Organize SDK files in IDE
        source_group(sdk FILES ${FORMAT_SDK_FILES})

        add_library(${SUB_PROJECT_NAME} MODULE
            ${GMPI_PLUGIN_SOURCE_FILES}
            ${FORMAT_SDK_FILES}
            ${resource_srcs}
            ${wrapper_srcs}
        )

        # Target-based includes
        if(plugin_includes)
            target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${plugin_includes})
        endif()
        # Adaptor headers
        if(GMPI_ADAPTORS)
            target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${GMPI_ADAPTORS})
        endif()

        gmpi_target(PROJECT_NAME ${SUB_PROJECT_NAME})
        
        set(TARGET_EXTENSION "${kind}")
        if(kind STREQUAL "AU")
            set(TARGET_EXTENSION "component")
            add_dependencies(${SUB_PROJECT_NAME} plist_util ${GMPI_PLUGIN_PROJECT_NAME}) # ensure plist util and GMPI plugin are built first.

            # Generate Info.plist using plist_util by scanning the GMPI bundle
            add_custom_command(TARGET ${SUB_PROJECT_NAME} PRE_LINK
                COMMAND $<TARGET_FILE:plist_util>
                        "$<TARGET_BUNDLE_DIR:${GMPI_PLUGIN_PROJECT_NAME}>"  # scan input (GMPI bundle)
                        "${PLIST_OUT}" 
                # ARGS "$<TARGET_BUNDLE_DIR:${GMPI_PLUGIN_PROJECT_NAME}>" "${PLIST_OUT}"  # input bundle to scan, output plist
                # DEPENDS plist_util  ${GMPI_PLUGIN_PROJECT_NAME}                         # build tool and GMPI first                                                     
                # BYPRODUCTS "${PLIST_OUT}"
                COMMENT "Generating Info.plist for AU plugin"
                VERBATIM
            )
            
            # Tell Xcode/Bundle step to use the generated plist
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST "${PLIST_OUT}"
                BUNDLE TRUE
                BUNDLE_EXTENSION "component"
            )
        endif()
            
        if(APPLE)
            set_target_properties(${SUB_PROJECT_NAME}
            PROPERTIES
                BUNDLE TRUE
                BUNDLE_EXTENSION "${TARGET_EXTENSION}"
        )
        else()
            set_target_properties(${SUB_PROJECT_NAME}
                PROPERTIES
                SUFFIX ".${TARGET_EXTENSION}"
            )
        endif()
    endforeach()

    list(FIND GMPI_PLUGIN_FORMATS_LIST "GMPI" FIND_GMPI_INDEX)
    list(FIND GMPI_PLUGIN_FORMATS_LIST "VST3" FIND_VST3_INDEX)
    list(FIND GMPI_PLUGIN_FORMATS_LIST "AU" FIND_AU_INDEX)

    if(FIND_VST3_INDEX GREATER_EQUAL 0)
        set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_VST3)

        if(APPLE)
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "vst3")

            if(GMPI_PLUGIN_HAS_XML)
                # Place xml file in bundle 'Resources' folder.
                set(xml_path "${SUB_PROJECT_NAME}.xml")
                target_sources(${SUB_PROJECT_NAME} PUBLIC ${xml_path})
                set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            endif()
        else()
            set_target_properties(${SUB_PROJECT_NAME} PROPERTIES SUFFIX ".vst3")
        endif()

        # Link the VST3 wrapper as static libs
        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE base pluginterfaces VST3_Wrapper)
    endif()

    if(FIND_AU_INDEX GREATER_EQUAL 0)
        set(SUB_PROJECT_NAME ${GMPI_PLUGIN_PROJECT_NAME}_AU)
        
        # Link the AU2 wrapper as static libs
        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE base pluginterfaces AU_Wrapper)
        target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${AU_SDK_H})

        # copy plugin to components folder. NOTE: Requires user to have read-write permissions on folder.
        if(SE_LOCAL_BUILD)
            SET(AU_DEST "/Library/Audio/Plug-Ins/Components")
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

    if(WIN32 AND SE_LOCAL_BUILD)
        if(FIND_VST3_INDEX GREATER_EQUAL 0)
            add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME}_VST3
                POST_BUILD
                COMMAND copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\VST3\\$(TargetName)$(TargetExt)"
                COMMENT "Copy to VST3 folder"
                VERBATIM
            )
            set_target_properties(${GMPI_PLUGIN_PROJECT_NAME}_VST3 PROPERTIES FOLDER "VST3 plugins")
        endif()

        if(FIND_GMPI_INDEX GREATER_EQUAL 0)
            if(GMPI_PLUGIN_IS_OFFICIAL_MODULE)
                add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME}
                    POST_BUILD
                    COMMAND xcopy /c /y "\"$(OutDir)$(TargetName)$(TargetExt)\"" "\"C:\\SE\\SE16\\SynthEdit2\\mac_assets\\$(TargetName)$(TargetExt)\\Contents\\x86_64-win\\\""
                    COMMENT "Copy to SynthEdit plugin folder"
                    VERBATIM
                )
            else()
                add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME}
                    POST_BUILD
                    COMMAND copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\SynthEdit\\modules\\$(TargetName)$(TargetExt)"
                    COMMENT "Copy to GMPI folder"
                    VERBATIM
                )
            endif()
        endif()
    endif()

    # Group modules under solution folders
    if(FIND_GMPI_INDEX GREATER_EQUAL 0)
        set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES FOLDER "GMPI plugins")
    endif()
endfunction()

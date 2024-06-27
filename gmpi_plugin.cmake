function(GMPI_PLUGIN)
set(options HAS_DSP HAS_GUI HAS_XML BUILD_VST3_WRAPPER)
set(oneValueArgs PROJECT_NAME)
set(multiValueArgs SOURCE_FILES)
cmake_parse_arguments(GMPI_PLUGIN "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

# message(STATUS "PROJECT_NAME:" ${GMPI_PLUGIN_PROJECT_NAME})

# add SDK files
set(sdk_srcs
${gmpi_sdk_folder}/Core/Common.h
${gmpi_sdk_folder}/Core/Common.cpp
)

if(${GMPI_PLUGIN_HAS_DSP})
    set(sdk_srcs ${sdk_srcs}
    ${gmpi_sdk_folder}/Core/AudioPlugin.h
    ${gmpi_sdk_folder}/Core/AudioPlugin.cpp
    )
    set(srcs ${srcs}
    ${GMPI_PLUGIN_PROJECT_NAME}.cpp
    )
endif()

if(${GMPI_PLUGIN_HAS_GUI})
    set(sdk_srcs ${sdk_srcs}
    ${gmpi_ui_folder}/GmpiApiDrawing.h
    ${gmpi_ui_folder}/Drawing.h
    )
    set(srcs ${srcs}
    ${GMPI_PLUGIN_PROJECT_NAME}Gui.cpp
    )

    # add include folder
    include_directories(
        ${gmpi_ui_folder}
    )
endif()

if(${GMPI_PLUGIN_HAS_XML})
set(resource_srcs
${GMPI_PLUGIN_PROJECT_NAME}.xml
)

if(CMAKE_HOST_WIN32)
set(resource_srcs
    ${resource_srcs}
    ${GMPI_PLUGIN_PROJECT_NAME}.rc
)
source_group(resources FILES ${resource_srcs})
endif()
endif()

# organise SDK file into folders/groups in IDE
source_group(sdk FILES ${sdk_srcs})

include (GenerateExportHeader)
add_library(${GMPI_PLUGIN_PROJECT_NAME} MODULE ${GMPI_PLUGIN_SOURCE_FILES} ${sdk_srcs} ${resource_srcs})

target_compile_definitions(
  ${GMPI_PLUGIN_PROJECT_NAME} PRIVATE 
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

if(APPLE)
  set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES BUNDLE TRUE)

if(${GMPI_PLUGIN_HAS_XML})
  # Place xml file in bundle 'Resources' folder.
  set(xml_path "${GMPI_PLUGIN_PROJECT_NAME}.xml")
  target_sources(${GMPI_PLUGIN_PROJECT_NAME} PUBLIC ${xml_path})
  set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()
endif()

if(WIN32)
target_link_options(${GMPI_PLUGIN_PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
endif()

if(${GMPI_PLUGIN_BUILD_VST3_WRAPPER})
 #LINK THE VST3 wrapper as a static library (I changed the wrapper CMakeLists.txt to "add_library(${PROJECT_NAME} STATIC"" to make this work)
 target_link_libraries(${GMPI_PLUGIN_PROJECT_NAME} PUBLIC SynthEdit_VST3)
 # TODO: rename as gmpi_vst3_adaptor)
 target_compile_definitions(
  ${GMPI_PLUGIN_PROJECT_NAME} PRIVATE 
  GMPI_HAS_VST3_WRAPPER
)

set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES SUFFIX ".vst3")
set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "vst3")

else()

set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES SUFFIX ".gmpi")
set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "gmpi")

endif()

if(CMAKE_HOST_WIN32)

if (SE_LOCAL_BUILD)

# Run after all other rules within the target have been executed
if(${GMPI_PLUGIN_BUILD_VST3_WRAPPER})
    add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME}
    POST_BUILD
    COMMAND copy "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\VST3\\$(TargetName).vst3" /Y
    COMMENT "Copy to VST3 folder"
    VERBATIM
)

else()
    add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME}
    POST_BUILD
    COMMAND xcopy /c /y "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\SynthEdit\\modules\\"
    COMMENT "Copy to SEM folder"
    VERBATIM
)
endif()
endif()
endif()

# all individual modules should be groups under "modules" solution folder
SET_TARGET_PROPERTIES(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES FOLDER "modules")

endfunction()

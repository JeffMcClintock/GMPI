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

########################################### VST3 WRAPPER ###########################################
if(${GMPI_PLUGIN_BUILD_VST3_WRAPPER})
add_definitions(-D_UNICODE)
add_definitions(-DSE_TARGET_PLUGIN)
add_definitions(-DSE_TARGET_VST3)
add_definitions(-DGMPI_VST3_WRAPPER)

set(wrapper_srcs
${GMPI_ADAPTORS}/VST3/MyVstPluginFactory.h
${GMPI_ADAPTORS}/VST3/MyVstPluginFactory.cpp
${GMPI_ADAPTORS}/VST3/factory.cpp
${GMPI_ADAPTORS}/VST3/StagingMemoryBuffer.h
${GMPI_ADAPTORS}/VST3/StagingMemoryBuffer.h
${GMPI_ADAPTORS}/VST3/adelayprocessor.h
${GMPI_ADAPTORS}/VST3/adelayprocessor.cpp
${GMPI_ADAPTORS}/VST3/adelaycontroller.h
${GMPI_ADAPTORS}/VST3/adelaycontroller.cpp
${GMPI_ADAPTORS}/VST3/interThreadQue.h
${GMPI_ADAPTORS}/VST3/interThreadQue.cpp
${GMPI_ADAPTORS}/VST3/HostControls.h
${GMPI_ADAPTORS}/VST3/HostControls.cpp
${GMPI_ADAPTORS}/VST3/my_input_stream.h
${GMPI_ADAPTORS}/VST3/my_input_stream.cpp
${GMPI_ADAPTORS}/VST3/interThreadQue.cpp
${GMPI_ADAPTORS}/VST3/interThreadQue.h
${GMPI_ADAPTORS}/VST3/tinyxml2/tinyxml2.h
${GMPI_ADAPTORS}/VST3/tinyxml2/tinyxml2.cpp
${GMPI_ADAPTORS}/Common/dynamic_linking.h
${GMPI_ADAPTORS}/Common/dynamic_linking.cpp
${GMPI_ADAPTORS}/Common/MpParameter.h
${GMPI_ADAPTORS}/Common/MpParameter.cpp
${GMPI_ADAPTORS}/Common/GmpiResourceManager.h
${GMPI_ADAPTORS}/Common/GmpiResourceManager.cpp
${GMPI_ADAPTORS}/Common/platform_plugin.cpp
${GMPI_ADAPTORS}/Common/Controller.h
${GMPI_ADAPTORS}/Common/Controller.cpp
${GMPI_ADAPTORS}/Shared/MyTypeTraits.h
${GMPI_ADAPTORS}/Shared/MyTypeTraits.cpp
${GMPI_ADAPTORS}/Shared/RawConversions.h
${GMPI_ADAPTORS}/Shared/RawConversions.cpp
${GMPI_ADAPTORS}/Shared/BundleInfo.h
${GMPI_ADAPTORS}/Shared/BundleInfo.cpp
${GMPI_ADAPTORS}/Shared/ProcessorStateManager.h
${GMPI_ADAPTORS}/Shared/ProcessorStateManager.cpp
${GMPI_ADAPTORS}/Shared/conversion.h
${GMPI_ADAPTORS}/Shared/conversion.cpp
${GMPI_ADAPTORS}/Shared/FileFinder.h
${GMPI_ADAPTORS}/Shared/FileFinder.cpp
${GMPI_ADAPTORS}/Shared/FileWatcher.h
${GMPI_ADAPTORS}/Shared/FileWatcher.cpp
${GMPI_ADAPTORS}/Shared/it_enum_list.h
${GMPI_ADAPTORS}/Shared/it_enum_list.cpp

${gmpi_ui_folder}/helpers/TimerManager.h
${gmpi_ui_folder}/helpers/TimerManager.cpp

${VST3_SDK}/public.sdk/source/common/pluginview.h
${VST3_SDK}/public.sdk/source/common/pluginview.cpp
${VST3_SDK}/public.sdk/source/common/commoniids.cpp
    
${VST3_SDK}/public.sdk/source/vst/vstbus.h
${VST3_SDK}/public.sdk/source/vst/vstbus.cpp
${VST3_SDK}/public.sdk/source/vst/vstcomponentbase.h
${VST3_SDK}/public.sdk/source/vst/vstcomponentbase.cpp

${VST3_SDK}/public.sdk/source/vst/vsteditcontroller.h
${VST3_SDK}/public.sdk/source/vst/vsteditcontroller.cpp
${VST3_SDK}/public.sdk/source/vst/vstinitiids.cpp

${VST3_SDK}/public.sdk/source/vst/vstcomponent.h
${VST3_SDK}/public.sdk/source/vst/vstcomponent.cpp
${VST3_SDK}/public.sdk/source/vst/vstaudioeffect.h
${VST3_SDK}/public.sdk/source/vst/vstaudioeffect.cpp
${VST3_SDK}/public.sdk/source/vst/vstparameters.h
${VST3_SDK}/public.sdk/source/vst/vstparameters.cpp 
)

IF(WIN32)
set(wrapper_srcs ${wrapper_srcs}
${GMPI_ADAPTORS}/VST3/SEVSTGUIEditorWin.h
${GMPI_ADAPTORS}/VST3/SEVSTGUIEditorWin.cpp
${gmpi_ui_folder}/backends/DrawingFrameWin.h
${gmpi_ui_folder}/backends/DrawingFrameWin.cpp
${gmpi_ui_folder}/backends/DirectXGfx.h
${gmpi_ui_folder}/backends/DirectXGfx.cpp
${GMPI_ADAPTORS}/VST3/adelay.def
${VST3_SDK}/public.sdk/source/main/dllmain.cpp
)
ELSE()
set(wrapper_srcs ${wrapper_srcs}
${GMPI_ADAPTORS}/VST3/SEVSTGUIEditorMac.h
${GMPI_ADAPTORS}/VST3/SEVSTGUIEditorMac.cpp
${gmpi_ui_folder}/backends/DrawingFrameMac.mm
${gmpi_ui_folder}/backends/CocoaGfx.h
${gmpi_ui_folder}/backends/Gfx_base.h
${gmpi_ui_folder}/backends/Bezier.h
${VST3_SDK}/public.sdk/source/main/macmain.cpp
)
ENDIF()

source_group(wrapper FILES ${wrapper_srcs})

#${CMAKE_CURRENT_SOURCE_DIR}
include_directories(
${GMPI_ADAPTORS}/VST3
${GMPI_ADAPTORS}/common
${GMPI_ADAPTORS}/Shared
${VST3_SDK}
${DSP_CORE}
${gmpi_sdk_folder}/Core
${gmpi_ui_folder}
${gmpi_ui_folder}/helpers
)

if(APPLE)
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)
FIND_LIBRARY(QUARTZCORE_LIBRARY QuartzCore )
MARK_AS_ADVANCED (QUARTZCORE_LIBRARY)
FIND_LIBRARY(COCOA_LIBRARY Cocoa )
MARK_AS_ADVANCED (COCOA_LIBRARY)
FIND_LIBRARY(CORETEXT_LIBRARY CoreText )
MARK_AS_ADVANCED (CORETEXT_LIBRARY)
FIND_LIBRARY(OPENGL_LIBRARY OpenGL )
MARK_AS_ADVANCED (OPENGL_LIBRARY)
FIND_LIBRARY(IMAGEIO_LIBRARY ImageIO )
MARK_AS_ADVANCED (IMAGEIO_LIBRARY)
FIND_LIBRARY(COREGRAPHICS_LIBRARY CoreGraphics )
MARK_AS_ADVANCED (COREGRAPHICS_LIBRARY)
FIND_LIBRARY(ACCELERATE_LIBRARY Accelerate )
MARK_AS_ADVANCED (ACCELERATE_LIBRARY)
endif()

else()
endif()
########################################### VST3 WRAPPER ###########################################


add_library(${GMPI_PLUGIN_PROJECT_NAME} MODULE ${GMPI_PLUGIN_SOURCE_FILES} ${sdk_srcs} ${resource_srcs} ${wrapper_srcs})

target_compile_definitions(
  ${GMPI_PLUGIN_PROJECT_NAME} PRIVATE 
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

if(${GMPI_PLUGIN_BUILD_VST3_WRAPPER})
# 'DEBUG' is a steinberg thing, ('_DEBUG' is MSVC)
target_compile_definitions(${GMPI_PLUGIN_PROJECT_NAME} PRIVATE 
  $<$<CONFIG:Debug>:DEBUG=1>
  $<$<CONFIG:Release>:DEBUG=0>
)

TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${COCOA_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${CORETEXT_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${OPENGL_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${IMAGEIO_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${COREGRAPHICS_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${ACCELERATE_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} ${QUARTZCORE_LIBRARY} )

TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} base )
TARGET_LINK_LIBRARIES( ${GMPI_PLUGIN_PROJECT_NAME} pluginterfaces )

if(CMAKE_HOST_WIN32)
target_link_libraries(${GMPI_PLUGIN_PROJECT_NAME} d3d11.lib d2d1 dwrite windowscodecs)
endif()

if(APPLE)
set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES BUNDLE TRUE)
set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION vst3)
#  set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES PREFIX )

  # Place module xml in bundle 'Resources' folder.
#  set(xml_path ${GMPI_PLUGIN_PROJECT_NAME}.xml)
#  target_sources(${GMPI_PLUGIN_PROJECT_NAME} PUBLIC ${xml_path})
#  set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

if(WIN32)
target_link_options(${GMPI_PLUGIN_PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
endif()

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
endif()

#if(${GMPI_PLUGIN_BUILD_VST3_WRAPPER})
# #LINK THE VST3 wrapper as a static library (I changed the wrapper CMakeLists.txt to "add_library(${PROJECT_NAME} STATIC"" to make this work)
# target_link_libraries(${GMPI_PLUGIN_PROJECT_NAME} PUBLIC SynthEdit_VST3)
# # TODO: rename as gmpi_vst3_adaptor)
# target_compile_definitions(
#  ${GMPI_PLUGIN_PROJECT_NAME} PRIVATE 
#  GMPI_HAS_VST3_WRAPPER
#)
#set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES SUFFIX ".gmpi")
#set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "gmpi")
#
#else()
#
#set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES SUFFIX ".vst3")
#set_target_properties(${GMPI_PLUGIN_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "vst3")
#
#endif()

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
cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

project(SynthEdit_VST3)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-D_UNICODE)
add_definitions(-DSE_TARGET_PLUGIN)
add_definitions(-DSE_TARGET_VST3)

if (MSVC)
    # Floating Point Model: Fast (/fp:fast)
    # Buffer Security Check: No (/GS-)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GS-")
endif()

set(DSP_CORE ${CMAKE_CURRENT_SOURCE_DIR}/../SE_DSP_CORE)
#set(VST3_SDK ${CMAKE_CURRENT_SOURCE_DIR}/../SDKs/VST3_SDK)
    
set(srcs
factory.cpp
MyVstPluginFactory.h
MyVstPluginFactory.cpp
StagingMemoryBuffer.h
StagingMemoryBuffer.h
adelayprocessor.h
adelayprocessor.cpp
adelaycontroller.h
adelaycontroller.cpp

it_enum_list.h
it_enum_list.cpp
MpParameter.h
MpParameter.cpp
Controller.h
Controller.cpp
interThreadQue.h
interThreadQue.cpp
TimerManager.h
TimerManager.cpp
FileFinder.h
FileFinder.cpp
FileWatcher.h
FileWatcher.cpp
conversion.h
conversion.cpp
HostControls.h
HostControls.cpp
BundleInfo.h
BundleInfo.cpp
RawConversions.h
RawConversions.cpp
DrawingFrame_win32.h
DrawingFrame_win32.cpp
my_input_stream.h
my_input_stream.cpp
xp_dynamic_linking.h
xp_dynamic_linking.cpp
DirectXGfx.h
DirectXGfx.cpp
mp_sdk_common.h
mp_sdk_common.cpp
gmpi_gui_hosting.h
gmpi_gui_hosting.cpp
tinyxml2/tinyxml2.h
tinyxml2/tinyxml2.cpp
)
#ContainerView.h
#ContainerView.cpp

#adelay.def
IF(WIN32)
set(srcs ${srcs}
SEVSTGUIEditorWin.h
SEVSTGUIEditorWin.cpp
${VST3_SDK}/public.sdk/source/main/dllmain.cpp
adelay.def
)
ELSE()
set(srcs ${srcs}
../se_vst3/source/SEVSTGUIEditorMac.h
../se_vst3/source/SEVSTGUIEditorMac.cpp
${DSP_CORE}/modules/se_sdk3_hosting/EventHelper.h
${DSP_CORE}/modules/se_sdk3_hosting/CocoaGuiHost.h
${DSP_CORE}/modules/se_sdk3_hosting/CocoaGuiHost.mm
${DSP_CORE}/modules/se_sdk3_hosting/SynthEditCocoaView.h
${DSP_CORE}/modules/se_sdk3_hosting/SynthEditCocoaView.mm
${VST3_SDK}/public.sdk/source/main/macmain.cpp
)
ENDIF()

set(vst_gui_minimal_srcs
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

include_directories(
${sdk_folder}
${DSP_CORE}
${VST3_SDK}
)

source_group(se_sdk FILES ${sdk_srcs})

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

include (GenerateExportHeader)
add_library(${PROJECT_NAME} MODULE ${sdk_srcs} ${srcs} ${vst_gui_minimal_srcs})

# 'DEBUG' is a steinberg thing, ('_DEBUG' is standard C++)
target_compile_definitions(${PROJECT_NAME} PRIVATE 
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Debug>:DEBUG=1>
  $<$<CONFIG:Release>:DEBUG=0>
  $<$<CONFIG:Release>:NDEBUG>
)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COCOA_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${CORETEXT_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${OPENGL_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${IMAGEIO_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREGRAPHICS_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ACCELERATE_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${QUARTZCORE_LIBRARY} )

#TARGET_LINK_LIBRARIES( ${PROJECT_NAME} SynthEditLib )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} base )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} pluginterfaces )

if(CMAKE_HOST_WIN32)
target_link_libraries(${PROJECT_NAME} d3d11.lib d2d1 dwrite windowscodecs)
endif()

target_compile_definitions (${PROJECT_NAME} PRIVATE -D_UNICODE -DUNICODE)

if(APPLE)
set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE TRUE)
set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION vst3)
#  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX )

  # Place module xml in bundle 'Resources' folder.
#  set(xml_path ${PROJECT_NAME}.xml)
#  target_sources(${PROJECT_NAME} PUBLIC ${xml_path})
#  set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# windows dll extension
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".vst3")

########################### VST3 SDK #######################################
message(STATUS "VST3_SDK: ${VST3_SDK}")

list(APPEND CMAKE_MODULE_PATH "${VST3_SDK}/cmake/modules")

include(SMTG_VST3_SDK)

#---Add base libraries---------------------------
set(VST_SDK TRUE) # used for Base module which provides only a subset of Base for VST-SDK
add_subdirectory(${VST3_SDK}/pluginterfaces ${CMAKE_CURRENT_BINARY_DIR}/pluginterfaces)
add_subdirectory(${VST3_SDK}/base           ${CMAKE_CURRENT_BINARY_DIR}/base)

# this should be disabled on CI
if(SE_LOCAL_BUILD)
if(CMAKE_HOST_WIN32)
add_custom_command(TARGET ${PROJECT_NAME}
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND copy "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\VST3\\SynthEdit_VST3.vst3\\Contents\\x86_64-win"
    COMMENT "Copy to plugin folder"
    VERBATIM
)
endif()
endif()

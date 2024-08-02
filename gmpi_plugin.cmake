# setting that apply to every plugin format

function(gmpi_target)
set(options CAT)
set(oneValueArgs PROJECT_NAME)
set(multiValueArgs DOGS)
cmake_parse_arguments(GMPI_TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

target_compile_definitions(
  ${GMPI_TARGET_PROJECT_NAME} PRIVATE 
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

if(APPLE)
  set_target_properties(${GMPI_TARGET_PROJECT_NAME} PROPERTIES BUNDLE TRUE)
  set_target_properties(${GMPI_TARGET_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "gmpi")
else()
  set_target_properties(${GMPI_TARGET_PROJECT_NAME} PROPERTIES SUFFIX ".gmpi")
endif()

TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${COCOA_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${CORETEXT_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${OPENGL_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${IMAGEIO_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${COREGRAPHICS_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${ACCELERATE_LIBRARY} )
TARGET_LINK_LIBRARIES( ${GMPI_TARGET_PROJECT_NAME} ${QUARTZCORE_LIBRARY} )

if(CMAKE_HOST_WIN32)
target_link_libraries(${GMPI_TARGET_PROJECT_NAME} d3d11.lib d2d1 dwrite windowscodecs)
endif()

if(WIN32)
target_link_options(${GMPI_TARGET_PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
endif()

endfunction()

################################################################

function(gmpi_plugin)
set(options HAS_DSP HAS_GUI HAS_XML)
set(oneValueArgs PROJECT_NAME)
set(multiValueArgs FORMATS_LIST SOURCE_FILES)
cmake_parse_arguments(GMPI_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

# add SDK files
set(sdk_srcs
${gmpi_sdk_folder}/Core/Common.h
${gmpi_sdk_folder}/Core/Common.cpp
)

if(${GMPI_PLUGIN_HAS_DSP})
    set(sdk_srcs ${sdk_srcs}
    ${gmpi_sdk_folder}/Core/Processor.h
    ${gmpi_sdk_folder}/Core/Processor.cpp
    )
endif()

if(${GMPI_PLUGIN_HAS_GUI})
    set(sdk_srcs ${sdk_srcs}
    ${gmpi_ui_folder}/GmpiApiDrawing.h
    ${gmpi_ui_folder}/Drawing.h
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

foreach(kind IN LISTS GMPI_PLUGIN_FORMATS_LIST)
	set(SUB_PROJECT_NAME ${PROJECT_NAME}_${kind} )

    set(FORMAT_SDK_FILES ${sdk_srcs})

    if(kind STREQUAL "VST3")
        set(FORMAT_SDK_FILES ${sdk_srcs} ${GMPI_ADAPTORS}/wrapper/VST3/wrapperVst3.cpp)
        if(APPLE)
            set(FORMAT_SDK_FILES ${FORMAT_SDK_FILES} ${VST3_SDK}/public.sdk/source/main/macmain.cpp)
        endif()
    endif()
    
    source_group(sdk FORMAT_SDK_FILES ${sdk_srcs})

    add_library(${SUB_PROJECT_NAME} MODULE ${GMPI_PLUGIN_SOURCE_FILES} ${FORMAT_SDK_FILES} ${resource_srcs} ${wrapper_srcs})
    gmpi_target(PROJECT_NAME ${SUB_PROJECT_NAME})

endforeach()

list(FIND GMPI_PLUGIN_FORMATS_LIST "GMPI" FIND_GMPI_INDEX)
list(FIND GMPI_PLUGIN_FORMATS_LIST "VST3" FIND_VST3_INDEX)

if(FIND_VST3_INDEX GREATER_EQUAL 0)
	set(SUB_PROJECT_NAME ${PROJECT_NAME}_VST3 )

if(APPLE)
  set_target_properties(${SUB_PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "vst3")

if(${GMPI_TARGET_HAS_XML})
  # Place xml file in bundle 'Resources' folder.
  set(xml_path "${SUB_PROJECT_NAME}.xml")
  target_sources(${SUB_PROJECT_NAME} PUBLIC ${xml_path})
  set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif() # HAS_XML
else()
  set_target_properties(${SUB_PROJECT_NAME} PROPERTIES SUFFIX ".vst3")
endif() # APPLE

# LINK THE VST3 wrapper as a static library (I changed the wrapper CMakeLists.txt to "add_library(${PROJECT_NAME} STATIC"" to make this work)
# TODO: rename as gmpi_vst3_adaptor)
TARGET_LINK_LIBRARIES( ${SUB_PROJECT_NAME} base )
TARGET_LINK_LIBRARIES( ${SUB_PROJECT_NAME} pluginterfaces )
target_link_libraries(${SUB_PROJECT_NAME} VST3_Wrapper)

endif() # GMPI_PLUGIN_HAS_VST3

include_directories(
    ${GMPI_ADAPTORS}
)

if(CMAKE_HOST_WIN32)

if (SE_LOCAL_BUILD)

# Run after all other rules within the target have been executed
if(FIND_VST3_INDEX GREATER_EQUAL 0)
    add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME}_VST3
    POST_BUILD
    COMMAND copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\VST3\\$(TargetName)$(TargetExt)"
    COMMENT "Copy to VST3 folder"
    VERBATIM
)
SET_TARGET_PROPERTIES(${GMPI_PLUGIN_PROJECT_NAME}_VST3 PROPERTIES FOLDER "VST3 plugins")
endif()

if(FIND_GMPI_INDEX GREATER_EQUAL 0)
    add_custom_command(TARGET ${GMPI_PLUGIN_PROJECT_NAME}_GMPI
    POST_BUILD
    COMMAND copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\SynthEdit\\modules\\$(TargetName)$(TargetExt)"
    COMMENT "Copy to GMPI folder"
    VERBATIM
)
endif()
endif()
endif()

# all individual modules should be groups under "modules" solution folder
if(FIND_GMPI_INDEX GREATER_EQUAL 0)
    SET_TARGET_PROPERTIES(${GMPI_PLUGIN_PROJECT_NAME}_GMPI PROPERTIES FOLDER "GMPI plugins")
endif()

endfunction()

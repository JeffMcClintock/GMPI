cmake_minimum_required(VERSION 3.15)

project(Gain)

message(STATUS "foo include dir: ${sdk_folder}")

set(sdk_srcs
${sdk_folder}/Common.h
${sdk_folder}/Common.cpp
${sdk_folder}/AudioPlugin.h
${sdk_folder}/AudioPlugin.cpp
)
set(srcs
${PROJECT_NAME}.cpp
)
include_directories(
${sdk_folder}
)

source_group(gmpi FILES ${sdk_srcs})

if(APPLE)
#GUI Modules Only
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)
endif()

include (GenerateExportHeader)
add_library(${PROJECT_NAME} MODULE ${sdk_srcs} ${srcs})
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )
target_compile_definitions (${PROJECT_NAME} PRIVATE -D_UNICODE -DUNICODE)

if(APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX )
  set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE TRUE)
  set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION gmpi)
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .gmpi)

if(CMAKE_HOST_WIN32)
add_custom_command(TARGET ${PROJECT_NAME}
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND copy "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\SynthEdit\\modules\\"
    COMMENT "Copy to system plugin folder"
    VERBATIM
)
endif()

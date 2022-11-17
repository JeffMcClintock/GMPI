#pragma once
/*
#include "IPluginGui.h"
*/

#include <string>
#include <memory>

typedef int int32_t;

// ENSURE STAYS IN SYNC WITH MP_API.H
enum ParameterFieldType { FT_VALUE
                          , FT_SHORT_NAME
                          , FT_LONG_NAME
                          , FT_MENU_ITEMS
                          , FT_MENU_SELECTION
                          , FT_RANGE_LO
                          , FT_RANGE_HI
                          , FT_ENUM_LIST
                          , FT_FILE_EXTENSION
                          , FT_IGNORE_PROGRAM_CHANGE
                          , FT_PRIVATE
                          , FT_AUTOMATION			// int
                          , FT_AUTOMATION_SYSEX		// STRING
                          , FT_DEFAULT				// same type as parameter
                          , FT_GRAB					// (mouse down) bool
                          , FT_NORMALIZED			// float

                          , FT_HOST_PARAMETER = 50	// bool
						  , FT_VST_PARAMETER_INDEX
						  , FT_VST_DISPLAY_TYPE
						  , FT_VST_DISPLAY_MIN
						  , FT_VST_DISPLAY_MAX
						  , FT_END_OF_LIST
                        };

// helper function
int getFieldDatatype( ParameterFieldType fieldType );
int getFieldSize( ParameterFieldType fieldType );

// Plugin GUI interface
class IPluginGui
{
public:
	virtual int32_t OnParamUpdateFromHost(int handle, ParameterFieldType field, int voice, const void* data, int32_t size) = 0;
	virtual int32_t onParameterDeleted(int p_parameter_handle) = 0;
};



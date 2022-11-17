//#include "pch.h"
#include <assert.h>
#include "IPluginGui.h"
#include "se_datatypes.h"
#include "RawConversions.h"

int getFieldDatatype( ParameterFieldType fieldType )
{
	switch( fieldType )
	{
		// MIDI_LEARN_MENU_ITEMS
	case FT_ENUM_LIST:
	case FT_FILE_EXTENSION:
	case FT_MENU_ITEMS:
	case FT_SHORT_NAME:
	case FT_LONG_NAME:
	case FT_AUTOMATION_SYSEX:
		return DT_TEXT;
		break;

	case FT_GRAB:
	case FT_PRIVATE:
	case FT_HOST_PARAMETER:
	case FT_IGNORE_PROGRAM_CHANGE:
		return DT_BOOL;
		break;

	case FT_MENU_SELECTION:
	case FT_AUTOMATION:
	case FT_VST_PARAMETER_INDEX:
	case FT_VST_DISPLAY_TYPE:
		return DT_INT;
		break;

	case FT_NORMALIZED:
	case FT_VST_DISPLAY_MIN:
	case FT_VST_DISPLAY_MAX:
		return DT_FLOAT;
		break;

	case FT_RANGE_LO:
	case FT_RANGE_HI:
	case FT_VALUE:
		return -1; // uses parameter datatype.

	default:
		assert( false );
		return -1; // Missing enum!!
	};
}

int getFieldSize( ParameterFieldType fieldType )
{
	switch( fieldType )
	{
	case FT_NORMALIZED:
	case FT_VST_DISPLAY_MIN:
	case FT_VST_DISPLAY_MAX:
		return RawSize((float) 0.0f);
		break;

	case FT_MENU_SELECTION:
	case FT_AUTOMATION:
	case FT_VST_PARAMETER_INDEX:
	case FT_VST_DISPLAY_TYPE:
		return RawSize((int)0);
		break;

	case FT_GRAB:
	case FT_IGNORE_PROGRAM_CHANGE:
	case FT_PRIVATE:
	case FT_HOST_PARAMETER:
		return RawSize( (bool) 0 );
		break;

	case FT_FILE_EXTENSION:
		return 0;

	default:
		assert(false);
		return -1;
		break;
	};
}


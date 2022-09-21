#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/funknown.h"
#include "GmpiApiCommon.h"

namespace tinyxml2
{
	class XMLElement;
}

struct pluginInfo
{
	std::string name_;
	std::string version_;
	std::string subCategories_;
	Steinberg::FUID processorId;
	Steinberg::FUID controllerId;
	int ElatencyCompensation;

	bool outputsAsStereoPairs;
	bool emulateIgnorePC = false;
	std::string outputNames;
};

struct pinInfoSem
{
	int32_t id;
	std::string name;
	gmpi::PinDirection direction;
	gmpi::PinDatatype datatype;
	std::string default_value;
	int32_t parameterId;
	int32_t flags;
	std::string hostConnect;
	std::string meta_data;
};
struct paramInfoSem
{
	int32_t id;
	std::string name;
	gmpi::PinDatatype datatype;
	std::string default_value;
	//int32_t parameterId;
	//int32_t flags;
	//std::string hostConnect;
	std::string meta_data;
	bool is_private;
};

struct pluginInfoSem
{
	std::string id;
	std::string name;
	int inputCount = {};
	int outputCount = {};
	std::vector<pinInfoSem> dspPins;
	std::vector<pinInfoSem> guiPins;
	std::vector<paramInfoSem> parameters;
};

inline int countPins(pluginInfoSem& plugin, gmpi::PinDirection direction, gmpi::PinDatatype datatype)
{
	return std::count_if(
		plugin.dspPins.begin()
		, plugin.dspPins.end()
		, [direction, datatype](const pinInfoSem& p) -> bool
		{
			return p.direction == direction && p.datatype == datatype;
		}
	);
}

inline auto getPinName(pluginInfoSem& plugin, gmpi::PinDirection direction, int index) -> std::string
{
	int i = 0;
	for (auto& p : plugin.dspPins)
	{
		if (p.direction != direction || p.datatype != gmpi::PinDatatype::Audio)
			continue;

		if (i++ == index)
			return p.name;
	}

	return {};
}

class MyVstPluginFactory :
	public Steinberg::IPluginFactory3
{
public:
	MyVstPluginFactory();
	~MyVstPluginFactory();

	static MyVstPluginFactory* GetInstance();

	/** Fill a PFactoryInfo structure with information about the Plug-in vendor. */
	virtual Steinberg::tresult PLUGIN_API getFactoryInfo (Steinberg::PFactoryInfo* info);

	/** Returns the number of exported classes by this factory. 
	If you are using the CPluginFactory implementation provided by the SDK, it returns the number of classes you registered with CPluginFactory::registerClass. */
	virtual Steinberg::int32 PLUGIN_API countClasses ();
	
	/** Fill a PClassInfo structure with information about the class at the specified index. */
	virtual Steinberg::tresult PLUGIN_API getClassInfo (Steinberg::int32 index, Steinberg::PClassInfo* info);
	
	/** Create a new class instance. */
	virtual Steinberg::tresult PLUGIN_API createInstance (Steinberg::FIDString cid, Steinberg::FIDString iid, void** obj);

	/** Returns the class info (version 2) for a given index. */
	virtual Steinberg::tresult PLUGIN_API getClassInfo2 (Steinberg::int32 index, Steinberg::PClassInfo2* info);

	int32_t getVst2Id(int32_t index); // not used. Same ID as 32-bit version.
	int32_t getVst2Id64(int32_t index); // generated from hash of GUID. Not compatible w 32-bit VSTs.

	/** Returns the unicode class info for a given index. */
	virtual Steinberg::tresult PLUGIN_API getClassInfoUnicode (Steinberg::int32 index, Steinberg::PClassInfoW* info);
	
	/** Receives information about host*/
	virtual Steinberg::tresult PLUGIN_API setHostContext (Steinberg::FUnknown* context);

/*
	DECLARE_FUNKNOWN_METHODS
*/
	// No Ref counting. Static object.
	virtual ::Steinberg::tresult PLUGIN_API queryInterface(const ::Steinberg::TUID iid, void** obj);
	virtual ::Steinberg::uint32 PLUGIN_API addRef(){
		return 1;
	}
	virtual ::Steinberg::uint32 PLUGIN_API release(){
		return 1;
	}

#if !defined( _WIN32 ) // Mac Only.
//    std::wstring getSemFolder(); // mac only.
#endif
	std::string getVendorName();
	std::string getProductName()
	{
		return pluginInfo_.name_;
	}
	bool GetOutputsAsStereoPairs();
	std::wstring GetOutputsName(int index);
	const pluginInfo& getPluginInfo()
	{
		return pluginInfo_;
	}

	std::vector<pluginInfoSem> plugins;

private:
	void initialize();
	void RegisterXml(const char* xml);
	void RegisterPin(tinyxml2::XMLElement* pin, std::vector<pinInfoSem>* pinlist, int32_t plugin_sub_type,
		int nextPinId);
	bool initializeFactory();

	std::string vendorName_;
	std::string vendorUrl_;
	std::string vendorEmail_;

	pluginInfo pluginInfo_;
	int32_t backwardCompatible4charId = -1;

};


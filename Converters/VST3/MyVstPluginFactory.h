#pragma once
#include <string>
#include <vector>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/funknown.h"

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

struct pluginInfoSem
{
	std::string id;
	std::string name;
};

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

private:
	void initialize();
	void RegisterXml(const char* xml);
	bool initializeFactory();

	std::string vendorName_;
	std::string vendorUrl_;
	std::string vendorEmail_;

	pluginInfo pluginInfo_;
	int32_t backwardCompatible4charId = -1;

	std::vector<pluginInfoSem> plugins;
};


/*
#include "BundleInfo.h"

BundleInfo::instance()->getResource("whatever");
*/

#ifndef BundleInstance_h
#define BundleInstance_h

#include <string>
#include <vector>
//#include "ElatencyContraintType.h"

#if __APPLE__
#include <AudioUnit/AudioUnit.h>
#endif

namespace wrapper
{
class BundleInfo
{
public:
    enum class Vst3MidiCcSupport { Auto, None, MPE_Emu, Chan_16 };
/*
    struct pluginInformation
    {
        int32_t pluginId = -1;
        std::string manufacturerId;
        int32_t inputCount;
        int32_t outputCount;
        int32_t midiInputCount;
        ElatencyContraintType latencyConstraint;
        std::string pluginName;
        std::string vendorName;
        std::string subCategories;
        bool outputsAsStereoPairs;
        bool emulateIgnorePC = {};
        bool monoUseOk;
        bool vst3Emulate16ChanCcs;
        std::vector<std::string> outputNames;
    };
 
private:
    pluginInformation info_ = {};
*/
    
    void initPluginInfo();
    BundleInfo();
    
public:
    inline static const char* resourceTypeScheme = "res://";
    std::wstring semFolder;
    std::wstring presetFolder; // native format

    bool pluginIsBundle = true;
    bool isEditor = false;
    
    static BundleInfo* instance();
    
#if __APPLE__
	CFBundleRef GetBundle();
    void setVendorName(const char* name)
    {
        info_.vendorName = name;
    }
#endif
    //void setPluginId(int32_t id)
    //{
    //    info_.pluginId = id;
    //}
    bool ResourceExists(const char* resourceId);
	std::string getResource(const char* resourceId);
	std::wstring getImbeddedFileFolder();
    std::wstring getResourceFolder();
	std::wstring getSemFolder();
    std::wstring getPresetFolder()
    {
        return presetFolder;
    }
	std::wstring getUserDocumentFolder();
//	int32_t getPluginId(); // 4-char VST2 code to identify presets.
//    const pluginInformation& getPluginInfo();
};
}
#endif /* BundleInstance_h */

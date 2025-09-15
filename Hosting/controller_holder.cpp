#include "Hosting/controller_holder.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include "tinyXml2/tinyxml2.h"

extern "C"
gmpi::ReturnCode MP_GetFactory(void** returnInterface);

namespace gmpi
{
namespace hosting
{

void gmpi_controller_holder::init(gmpi::hosting::pluginInfo& pinfo)
{
	info = &pinfo;
	patchManager.init(pinfo);

	// create a list of native params. the index must line up strictly with the parameter DAW tag
	for (auto& paramInfo : pinfo.parameters)
	{
		if (paramInfo.dawTag > -1)
		{
			assert(paramInfo.dawTag == nativeParams.size());
			nativeParams.push_back(&patchManager.parameters[paramInfo.id]);
		}
	}
}

void gmpi_controller_holder::initUi(gmpi::api::IEditor* gui)
{
	m_editors.push_back(gui);

	constexpr int32_t voice{};

	for (auto& pin : info->guiPins)
	{
		if (auto* param = patchManager.getParameter(pin.parameterId); param)
		{
			switch (pin.parameterFieldType)
			{
			case gmpi::Field::Normalized:
			{
				const float valueNormalized = static_cast<float>(param->normalisedValue());
				gui->setPin(pin.id, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
			}
			break;

			case gmpi::Field::Value:
			{
				switch (pin.datatype)
				{
				case gmpi::PinDatatype::Float32:
				{
					const float value = static_cast<float>(param->valueReal());
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
				}
				break;

				case gmpi::PinDatatype::Int32:
				{
					const int32_t value = static_cast<int32_t>(std::round(param->valueReal()));
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
				}
				break;

				case gmpi::PinDatatype::Bool:
				{
					const bool value = static_cast<bool>(std::round(param->valueReal()));
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
				}
				break;

				case gmpi::PinDatatype::Blob:
				{
					const auto& value = std::get<std::vector<uint8_t>>(param->value_); // param->value_;
					gui->setPin(pin.id, 0, static_cast<int32_t>(value.size()), value.data());
					gui->notifyPin(pin.id, voice);
				}
				break;

				default:
					assert(false); // unsupported type.
				}
				break;
			}
			}
		}
		else
		{
			// non-parameter pins
			const auto def = atof(pin.default_value.c_str());

			switch (pin.datatype)
			{
			case gmpi::PinDatatype::Float32:
			{
				const float value = static_cast<float>(def);
				gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
			}
			break;

			case gmpi::PinDatatype::Enum:
			case gmpi::PinDatatype::Int32:
			{
				const int32_t value = static_cast<int32_t>(std::round(def));
				gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
			}
			break;

			case gmpi::PinDatatype::Bool:
			{
				const bool value = static_cast<bool>(std::round(def));
				gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
			}
			break;

			case gmpi::PinDatatype::Blob:
			{
				// TODO !!! const bool value = static_cast<bool>(std::round(def));
				Blob test;
				gui->setPin(pin.id, 0, test.size(), test.data());
			}
			break;

			default:
				assert(false); // unsupported type.
			}
		}
	}
}

gmpi::ReturnCode gmpi_controller_holder::unRegisterGui(gmpi::api::IEditor* gui)
{
#if _HAS_CXX20
	std::erase(m_editors, gui);
#else
	if (auto it = find(m_editors.begin(), m_editors.end(), gui); it != m_editors.end())
		m_editors.erase(it);
#endif
	return gmpi::ReturnCode::Ok;
}

void gmpi_controller_holder::initUi(gmpi::api::IParameterObserver* gui)
{
	for (auto& pin : info->guiPins)
	{
		for (auto& param_info : info->parameters)
		{
			if (param_info.id == pin.parameterId)
			{
				if (auto* param = patchManager.getParameter(param_info.id); param)
				{
					switch (pin.parameterFieldType)
					{
					case gmpi::Field::Normalized:
					{
						const float valueNormalized = static_cast<float>(param->normalisedValue());
						gui->setParameter(param->info->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
					}
					break;

					case gmpi::Field::Value:
					{
						switch (pin.datatype)
						{
						case gmpi::PinDatatype::Float32:
						{
							const float value = static_cast<float>(param->valueReal());
							gui->setParameter(param->info->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Int32:
						{
							const int32_t value = static_cast<int32_t>(std::round(param->valueReal()));
							gui->setParameter(param->info->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Bool:
						{
							const bool value = static_cast<bool>(std::round(param->valueReal()));
							gui->setParameter(param->info->id, pin.parameterFieldType, 0, sizeof(bool), reinterpret_cast<const uint8_t*>(&value));
						}
						break;
						default:
							assert(false); // unsupported type.
						}
						break;
					}
					}

//					gmpi::hosting::my_msg_que_output_stream s(getQueueToDsp(), param->parameterHandle_, "ppc\0"); // "ppc"
				}
			}
		}
	}
}

void gmpi_controller_holder::notifyGui(GmpiParameter* param)
{
	constexpr int voice{};

	for (auto& pin : info->guiPins)
	{
		if (pin.parameterId != param->info->id)
			continue;

		for (auto* gui : m_editors)
		{
			switch (pin.parameterFieldType)
			{
			case gmpi::Field::Normalized:
			{
				const float valueNormalized = static_cast<float>(param->normalisedValue());
				gui->setPin(pin.id, voice, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
				gui->notifyPin(pin.id, voice);
			}
			break;

			case gmpi::Field::Value:
			{
				switch (pin.datatype)
				{
				case gmpi::PinDatatype::Float32:
				{
					const float value = static_cast<float>(param->valueReal());
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
					gui->notifyPin(pin.id, voice);
				}
				break;

				case gmpi::PinDatatype::Int32:
				{
					const int32_t value = static_cast<int32_t>(std::round(param->valueReal()));
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
					gui->notifyPin(pin.id, voice);
				}
				break;

				case gmpi::PinDatatype::Bool:
				{
					const bool value = static_cast<bool>(std::round(param->valueReal()));
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
					gui->notifyPin(pin.id, voice);
				}
				break;
				default:
					assert(false); // unsupported type.
				}
				break;
			}
			}
		}
	}
}

void gmpi_controller_holder::setPinFromUi(int32_t pinId, int32_t voice, std::span<const std::byte> data) //int32_t size, const void* data)
{
	for (auto& pin : info->guiPins)
	{
		if (pin.id == pinId && pin.parameterId != -1)
		{
			for (auto& param_info : info->parameters)
			{
				if (param_info.id == pin.parameterId)
				{
					if (auto* param = patchManager.getParameter(param_info.id); param)
					{
						bool changed{};

						switch (pin.parameterFieldType)
						{
						case gmpi::Field::Normalized:
						{
							assert(data.size() == sizeof(float));
							const float valueNormalized = *reinterpret_cast<const float*>(data.data());

							changed = param->setNormalised(static_cast<double>(valueNormalized));
						}
						break;

						case gmpi::Field::Value:
						{
							switch (pin.datatype)
							{
							case gmpi::PinDatatype::Float32:
							{
								assert(data.size() == sizeof(float));
								const float value = *reinterpret_cast<const float*>(data.data());
								changed = param->setReal(static_cast<double>(value));
							}
							break;

							case gmpi::PinDatatype::Int32:
							{
								assert(data.size() == sizeof(int32_t));
								const int32_t value = *reinterpret_cast<const int32_t*>(data.data());
								changed = param->setReal(static_cast<double>(value));
							}
							break;

							case gmpi::PinDatatype::Bool:
							{
								assert(data.size() == sizeof(bool));
								const bool value = *reinterpret_cast<const bool*>(data.data());
								changed = param->setReal(static_cast<double>(value));
							}
							break;
							default:
								assert(false); // unsupported type.
							}
							break;
						}
						}

						if(changed)
						{
							//// updateProcessor
							//pendingControllerQueueClients.AddWaiter(param);

#if 0 // not used currently
							// update parameter-watchers
							for (auto* gui : m_guis)
							{
								gui->setParameter(param->id, pin.parameterFieldType, voice, static_cast<int32_t>(data.size()), reinterpret_cast<const uint8_t*>(data.data()));
							}
#endif
							// editors too.
							for (auto* gui : m_editors)
							{
								for (auto& pin : info->guiPins)
								{
									for (auto& param_info : info->parameters)
									{
										if (param_info.id == pin.parameterId)
										{
											if (auto* param = patchManager.getParameter(param_info.id); param)
											{
												switch (pin.parameterFieldType)
												{
												case gmpi::Field::Normalized:
												{
													const float valueNormalized = static_cast<float>(param->normalisedValue());
													gui->setPin(pin.id, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
													//gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
												}
												break;

												case gmpi::Field::Value:
												{
													switch (pin.datatype)
													{
													case gmpi::PinDatatype::Float32:
													{
														const float value = static_cast<float>(param->valueReal());
														gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
														//							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
													}
													break;

													case gmpi::PinDatatype::Int32:
													{
														const int32_t value = static_cast<int32_t>(std::round(param->valueReal()));
														gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
														//							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
													}
													break;

													case gmpi::PinDatatype::Bool:
													{
														const bool value = static_cast<bool>(std::round(param->valueReal()));
														gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
														//							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(bool), reinterpret_cast<const uint8_t*>(&value));
													}
													break;
													default:
														assert(false); // unsupported type.
													}
													break;
												}
												}

												//					gmpi::hosting::my_msg_que_output_stream s(getQueueToDsp(), param->parameterHandle_, "ppc\0"); // "ppc"
											}
										}
									}
								}
							}

							// update DAW
							notifyDaw(param);
						}
					}

					//auto param = tagToParameter[pin.parameterId];
					//setParameterValue({ data, static_cast<size_t>(size) }, param->parameterHandle_, pin.parameterFieldType, voice);
					break;
				}
			}
			break;
		}
	}
}

bool gmpi_controller_holder::onQueMessageReady(int handle, int msg_id, gmpi::hosting::my_input_stream& strm)
{
	switch (msg_id)
	{
	case gmpi::hosting::id_to_long("ppc2"): // "ppc2" Patch parameter change, always sent as a double
	{
		constexpr int voice{};

		double val{};
		strm >> val;

		if (auto param = patchManager.setParameterReal(handle, val); param)
		{
#if 0 // not used currently
			// parameter watchers
			for (auto& pin : info->guiPins)
			{
				if (pin.parameterId == param->id)
				{

					for (auto* gui : m_guis)
					{
						const float valueNormalized = static_cast<float>(param->normalisedValue());
						gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));

//						gui->setParameter(param->id, pin.parameterFieldType, voice, static_cast<int32_t>(data.size()), reinterpret_cast<const uint8_t*>(data.data()));
						switch (pin.datatype)
						{
						case gmpi::PinDatatype::Float32:
						{
							const float value = static_cast<float>(param->valueReal);
							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Int32:
						{
							const int32_t value = static_cast<int32_t>(std::round(param->valueReal));
							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Bool:
						{
							const bool value = static_cast<bool>(std::round(param->valueReal));
							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(bool), reinterpret_cast<const uint8_t*>(&value));
						}
						break;
						default:
							assert(false); // unsupported type.
						}
						break;
					}
				}
			}
#endif
            constexpr int32_t voice{0};
            
            // editors too.
            for (auto* gui : m_editors)
            {
                for (auto& pin : info->guiPins)
                {
                    if (param->info->id != pin.parameterId)
                        continue;

                    switch (pin.parameterFieldType)
                    {
                    case gmpi::Field::Normalized:
                    {
                        const float valueNormalized = static_cast<float>(param->normalisedValue());
                        gui->setPin(pin.id, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
                        
                        gui->notifyPin(pin.id, voice);
                        //gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
                    }
                    break;

                    case gmpi::Field::Value:
                    {
                        switch (pin.datatype)
                        {
                        case gmpi::PinDatatype::Float32:
                        {
                            const float value = static_cast<float>(param->valueReal());
                            gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
                            gui->notifyPin(pin.id, voice);
                            //                            gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
                        }
                        break;

                        case gmpi::PinDatatype::Int32:
                        {
                            const int32_t value = static_cast<int32_t>(std::round(param->valueReal()));
                            gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
                            gui->notifyPin(pin.id, voice);
                            //                            gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
                        }
                        break;

                        case gmpi::PinDatatype::Bool:
                        {
                            const bool value = static_cast<bool>(std::round(param->valueReal()));
                            gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
                            gui->notifyPin(pin.id, voice);
                            //                            gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(bool), reinterpret_cast<const uint8_t*>(&value));
                        }
                        break;
                        default:
                            assert(false); // unsupported type.
                        }
                        break;
                    }
                    }
                }
            }
		}

		return true;
	}
	break;

	case gmpi::hosting::id_to_long("ppc3"): // "ppc3" Patch parameter change, always sent as a BLOB
	{
		constexpr int voice{};

		int32_t size{};
		strm >> size;
		std::vector<std::uint8_t> data(size);
		strm.Read(data.data(), size);

		if (auto param = patchManager.setParameterBlob(handle, data); param)
		{
			constexpr int32_t voice{ 0 };

			// editors too.
			for (auto* gui : m_editors)
			{
				for (auto& pin : info->guiPins)
				{
					if (param->info->id != pin.parameterId)
						continue;

					switch (pin.parameterFieldType)
					{
					case gmpi::Field::Normalized:
					{
						assert(false); // non scalar type.
						const float valueNormalized{};
						gui->setPin(pin.id, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
						gui->notifyPin(pin.id, voice);
					}
					break;

					case gmpi::Field::Value:
					{
						switch (pin.datatype)
						{
						case gmpi::PinDatatype::Blob:
						case gmpi::PinDatatype::String:
						{
							const auto& value = std::get<std::vector<uint8_t>>(param->value_); // param->value_;
							gui->setPin(pin.id, 0, static_cast<int32_t>(value.size()), value.data());
							gui->notifyPin(pin.id, voice);
						}
						break;
						default:
							assert(false); // unsupported type.
						}
						break;
					}
					}
				}
			}
		}

		return true;
	}
	break;
	};

	return false; // failed to consume message.
}

void gmpi_controller_holder::setPresetXmlFromDaw(const std::string& chunk)
{
	std::string name, category;

	tinyxml2::XMLDocument doc;
	doc.Parse(chunk.c_str());

	auto* parentXml = &doc;

	auto presetXml = parentXml->FirstChildElement("Preset");

	if (!presetXml)
		return;

	tinyxml2::XMLNode* parametersE{};
	parametersE = presetXml;

	auto presetXmlElement = presetXml->ToElement();

	// Query plugin's 4-char code. Presence Indicates also that preset format supports MIDI learn.
	int32_t fourCC = -1;
	int formatVersion = 0;
	{
		const char* hexcode{};
		if (tinyxml2::XML_SUCCESS == presetXmlElement->QueryStringAttribute("pluginId", &hexcode))
		{
			formatVersion = 1;
			try
			{
				fourCC = std::stoul(hexcode, nullptr, 16);
			}
			catch (...)
			{
				// who gives a f*ck
			}
		}
	}

	// !!! TODO: Check 4-char ID correct.

	const char* temp{};
	if (tinyxml2::XML_SUCCESS == presetXmlElement->QueryStringAttribute("category", &temp))
	{
		category = temp;
	}
	//presetXmlElement->QueryStringAttribute("name", &name);
	if (tinyxml2::XML_SUCCESS == presetXmlElement->QueryStringAttribute("name", &temp))
	{
		name = temp;
	}

	std::unordered_set<int32_t> parametersInPreset;

	// assuming we are passed "Preset.Parameters" node.
	for (auto ParamElement = presetXml->FirstChildElement("Param"); ParamElement; ParamElement = ParamElement->NextSiblingElement("Param"))
	{
		int paramHandle = -1;
		ParamElement->QueryIntAttribute("id", &paramHandle);

		assert(paramHandle != -1);

		// ignore unrecognised parameters
		auto it = patchManager.parameters.find(paramHandle);
		if (it == patchManager.parameters.end())
			continue;

		auto& param = (*it).second;

		parametersInPreset.insert(paramHandle);

		//??			if (info.ignoreProgramChange)
		//				continue;

		//auto& values = params[paramHandle];
		/* no stateful parameters in preset we assume
					if (!parameter.stateful_) // For VST2 wrapper aeffect ptr. prevents it being inadvertently zeroed.
						continue;

					// possibly need to handle elsewhere
					if (parameter.ignorePc_ && ignoreProgramChange) // a short time period after plugin loads, ignore-PC parameters will no longer overwrite existing value.
						continue;

		*/
		auto v = ParamElement->Attribute("val"); // todo instead of constructing a pointless string, what about just passing the char* to ParseToRaw()?

		if (!v)
			continue;

		const double value = std::stod(v);
		param.setReal(value);

		// This block seems messy. Should updating a parameter be a single function call?
					// (would need to pass 'updateProcessor')
		{
			// calls controller_->updateGuis(this, voice)
//				parameter->setParameterRaw(gmpi::Field::Value, (int32_t)raw.size(), raw.data(), voiceId);

			/* todo elsewhere
			// updated cached value.
			parameter->upDateImmediateValue();
			*/
		}

		// MIDI learn. part of a preset?????
		if (formatVersion > 0)
		{
			// TODO
			int32_t midiController = -1;
			ParamElement->QueryIntAttribute("MIDI", &midiController);

			const char* sysexU{};
			ParamElement->QueryStringAttribute("MIDI_SYSEX", &sysexU);
		}
	}

	// set any parameters missing from the preset to their default
	// except for preset name and category
	for (auto& [paramHandle, param] : patchManager.parameters)
	{
#if 0
		if (info.hostControl == HC_PROGRAM_NAME || HC_PROGRAM_CATEGORY == info.hostControl)
			continue;
#endif

		if (parametersInPreset.find(paramHandle) == parametersInPreset.end())
		{
//			auto& values = params[paramHandle];

//			values.dataType = info.dataType;
//			for (const auto& v : info.defaultRaw)
			{
//				values.rawValues_.push_back(v);

				param.setToDefault();
			}
		}
	}

//#ifdef _DEBUG
//	for (auto& p : params)
//	{
//		assert(p.second.rawValues_.size() == 1);
//	}
//#endif

//	calcHash();
}

std::string gmpi_controller_holder::getPreset()
{
	tinyxml2::XMLDocument doc;
	// TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
	doc.LinkEndChild(doc.NewDeclaration());

	auto element = doc.NewElement("Preset");
	doc.LinkEndChild(element);

#if 0 // TODO
	{
		char txt[20];
#if defined(_MSC_VER)
		sprintf_s(txt, "%08x", pluginId);
#else
		snprintf(txt, std::size(txt), "%08x", pluginId);
#endif
		element->SetAttribute("pluginId", txt);
	}

	if (!presetNameOverride.empty())
	{
		element->SetAttribute("name", presetNameOverride.c_str());
	}
	else if (!name.empty())
	{
		element->SetAttribute("name", name.c_str());
	}

	if (!category.empty())
		element->SetAttribute("category", category.c_str());
#endif

	for (auto& [handle, parameter] : patchManager.parameters)
	{
		auto paramElement = doc.NewElement("Param");
		element->LinkEndChild(paramElement);
		paramElement->SetAttribute("id", handle);

		//const int voice = 0;
		//auto& raw = parameter.rawValues_[voice];

		//const auto val = RawToUtf8B(parameter.dataType, raw.data(), raw.size());

		paramElement->SetAttribute("val", parameter.valueReal());

#if 0  // TODO??
		// MIDI learn.
		if (parameter->MidiAutomation != -1)
		{
			paramElement->SetAttribute("MIDI", parameter->MidiAutomation);

			if (!parameter->MidiAutomationSysex.empty())
				paramElement->SetAttribute("MIDI_SYSEX", WStringToUtf8(parameter->MidiAutomationSysex));
		}
#endif
	}

	tinyxml2::XMLPrinter printer;
	// printer.SetIndent(" ");
	doc.Accept(&printer);

	return printer.CStr();
}


} // namespace hosting
} // namespace gmpi

#include "Hosting/controller_holder.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cstdio>
#include <cstring>
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

void gmpi_controller_holder::initUi(gmpi::api::IUnknown* unknownEditor)
{
	// we're borrowing 'unknownEditor', not adopting it: queryInterface addRefs,
	// whereas constructing a shared_ptr from the raw pointer would steal the caller's reference.
	gmpi::shared_ptr<gmpi::api::IEditor> editor;
	if (unknownEditor)
		unknownEditor->queryInterface(&gmpi::api::IEditor::guid, editor.put_void());

	if(editor.isNull())
	{
		assert(false); // not an IEditor
		return;
	}
	
	m_editors.push_back(editor.get());

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
				editor->setPin(pin.id, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&valueNormalized));
			}
			break;

			case gmpi::Field::Value:
			{
				switch (pin.datatype)
				{
				case gmpi::PinDatatype::Float32:
				{
					const float value = static_cast<float>(param->valueReal());
					editor->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
				}
				break;

				case gmpi::PinDatatype::Int32:
				{
					const int32_t value = static_cast<int32_t>(std::round(param->valueReal()));
					editor->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
				}
				break;

				case gmpi::PinDatatype::Bool:
				{
					const bool value = static_cast<bool>(std::round(param->valueReal()));
					editor->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
				}
				break;

				// A String parameter holds a vector<uint8_t> in the same variant
				// arm a Blob does (see GmpiParameter::setToDefault), and a
				// String PIN takes the same raw bytes - which is what the
				// "ppc3" queue message and notifyGui both already hand it. This
				// arm had Blob alone, so a plug-in whose editor read a text pin
				// got its initial value from nowhere and asserted below.
				case gmpi::PinDatatype::String:
				case gmpi::PinDatatype::Blob:
				{
					const auto& value = std::get<std::vector<uint8_t>>(param->value_); // param->value_;
					editor->setPin(pin.id, 0, static_cast<int32_t>(value.size()), value.data());
					editor->notifyPin(pin.id, voice);
				}
				break;

				default:
					assert(false); // unsupported type.
					break;
				}
			}
			break;

			default:
				assert(false); // unsupported type.
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
				editor->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
			}
			break;

			case gmpi::PinDatatype::Enum:
			case gmpi::PinDatatype::Int32:
			{
				const int32_t value = static_cast<int32_t>(std::round(def));
				editor->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
			}
			break;

			case gmpi::PinDatatype::Bool:
			{
				const bool value = static_cast<bool>(std::round(def));
				editor->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
			}
			break;

			case gmpi::PinDatatype::Blob:
			{
				// TODO !!! const bool value = static_cast<bool>(std::round(def));
				Blob test;
				editor->setPin(pin.id, 0, static_cast<int32_t>(test.size()), test.data());
			}
			break;

			default:
				assert(false); // unsupported type.
			}
		}
	}
}

gmpi::ReturnCode gmpi_controller_holder::unRegisterGui(gmpi::api::IUnknown* editor)
{
#if _HAS_CXX20
	std::erase(m_editors, editor);
#else
	if (auto it = find(m_editors.begin(), m_editors.end(), editor); it != m_editors.end())
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
				}
				break;

				default:
					assert(false); // unsupported
					break;
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

			case gmpi::PinDatatype::Blob:
			case gmpi::PinDatatype::String:
			{
				// The non-scalar arm, which used to fall through to the
				// assert below. Every OTHER push to an editor now has one -
				// initUi's Value switch and the "ppc3" queue message - so the
				// one path that lacked it was the one a host uses to say "this
				// parameter changed, tell the GUI", and any host that said it
				// about a blob asserted in a debug build. (initUi covered Blob
				// but not String; the missing label was added with this arm, so
				// the three agree rather than two of them agreeing.)
				//
				// Raw bytes, exactly as the "ppc3" arm of onQueMessageReady
				// delivers them: a String pin's value IS its bytes, and a Blob
				// pin's is whatever the plug-in put there. Base64 belongs to
				// the preset FILE, not to this in-memory hand-off.
				//
				// get_if rather than get, unlike the sites above: this arm is
				// reached for EVERY stateful parameter at once when a host
				// reverts a patch, so a spec whose pin datatype disagrees with
				// its parameter's would throw out of a UI update rather than
				// mismatching one parameter. Say so in a debug build and leave
				// the pin alone in a release one.
				const auto* value = std::get_if<std::vector<uint8_t>>(&param->value_);
				assert(value); // pin says text/binary, parameter holds a number

				if (value)
				{
					gui->setPin(pin.id, 0, static_cast<int32_t>(value->size()), value->data());
					gui->notifyPin(pin.id, voice);
				}
			}
			break;

			default:
				assert(false); // unsupported type.
			}
		}
		break;

		default:
			assert(false); // unsupported
			break;
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
					}
					break;

					default:
						assert(false); // unsupported
						break;
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
										}
										break;

										default:
											assert(false); // unsupported
											break;
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
                    }
					break;

					default:
						assert(false); // unsupported
						break;
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

		// Read the payload STRAIGHT INTO the parameter's own storage: one
		// copy, queue to value, no scratch buffer and no compare.
		//
		// NO DEDUP, and that is the whole point of this arm. A blob parameter
		// arriving here is a STREAM, not a value - TIDE's rack-feedback
		// channel carries whole `ppc` messages this way, and Scope's display
		// frames likewise. The gate this replaces (setParameterBlob returning
		// null when the bytes matched) silently swallowed identical batches,
		// which is not a lost repaint but LOST MESSAGES. Measured 2026-08-25:
		// the inner rack's feedback settled into a repeating 12-byte message,
		// every batch compared equal, every batch was dropped, and the editor
		// stopped receiving anything at all - module lights frozen, blinking
		// LED dead. Same rule as the processor-side blob arm; see setPin's
		// Blob case in processor_holder.cpp, which learned it first.
		//
		// Dropping the compare also costs nothing to skip: a compare that can
		// never gate anything is pure memory bandwidth.
		auto* param = patchManager.getParameter(handle);
		std::vector<uint8_t>* value{};
		if (param)
		{
			// A parameter whose variant still holds a number (a spec whose
			// datatype disagrees with what the DSP sends) gets converted
			// rather than skipped - setBlob used to do this, and silently
			// discarding every message instead is far worse than a surprise.
			if (!std::holds_alternative<std::vector<uint8_t>>(param->value_))
				param->value_.emplace<std::vector<uint8_t>>();

			value = std::get_if<std::vector<uint8_t>>(&param->value_);
		}

		if (value)
		{
			value->resize(static_cast<size_t>(size));   // keeps high-water capacity
			if (size > 0)
				strm.Read(value->data(), static_cast<unsigned int>(size));
		}
		else if (size > 0)
		{
			// Unknown handle: the framing still requires consuming the
			// payload, or every later message is read out of alignment.
			std::vector<uint8_t> discard(static_cast<size_t>(size));
			strm.Read(discard.data(), static_cast<unsigned int>(size));
		}

		if (value)
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
							gui->setPin(pin.id, 0, static_cast<int32_t>(value->size()), value->data());
							gui->notifyPin(pin.id, voice);
						}
						break;
						default:
							assert(false); // unsupported type.
						}
						break;
					}
					break;

					default:
						assert(false); // unsupported
						break;
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

bool gmpi_controller_holder::setPresetXmlFromDaw(const std::string& chunk)
{
	std::string name, category;

	tinyxml2::XMLDocument doc;
	doc.Parse(chunk.c_str());

	auto* parentXml = &doc;

	auto presetXml = parentXml->FirstChildElement("Preset");

	// No <Preset> element: garbage, an empty file, or a document of some other
	// kind. Nothing below has run, so the store is exactly as the caller left
	// it - said out loud rather than returned void, because a caller restoring
	// from a file has no other way to find out its file was not one.
	if (!presetXml)
		return false;

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
				fourCC = static_cast<int32_t>(std::stoul(hexcode, nullptr, 16));
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

		// The reader has to filter exactly as writePresetXml does, or the pair
		// is not a round trip. The writer skips non-stateful parameters
		// because they hold values good for one run only - a raw pointer to a
		// live object being the case that matters - so any <Param> naming one
		// came from a file written before that rule existed, or from another
		// tool. Taking it would overwrite a pointer the plug-in's controller
		// published moments ago during init() with a stale number from disk.
		//
		// It is also skipped from the reset-to-default pass below, for the
		// same reason and with the same effect: whatever is live stays live.
		if (!param.info->is_stateful)
			continue;

		parametersInPreset.insert(paramHandle);

		//??			if (info.ignoreProgramChange)
		//				continue;

		//auto& values = params[paramHandle];
		/*
					// possibly need to handle elsewhere
					if (parameter.ignorePc_ && ignoreProgramChange) // a short time period after plugin loads, ignore-PC parameters will no longer overwrite existing value.
						continue;

		*/
		auto v = ParamElement->Attribute("val"); // todo instead of constructing a pointless string, what about just passing the char* to ParseToRaw()?

		if (!v)
			continue;

		param.setFromXml(v);

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
		if(HostControls::None != param.info->hostConnect || !param.info->is_stateful)
			continue; // host connect only
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

	return true;
}

std::string writePresetXml(const std::unordered_map<int, GmpiParameter>& parameters)
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

	for (const auto& [handle, parameter] : parameters)
	{
		// Non-stateful parameters must never be written. They exist to carry
		// live, session-only values - TIDE's 'controllerPtr' is a blob holding
		// a raw pointer - and restoring one from a file hands the plug-in a
		// dangling pointer from a previous run. Harmless while blobs
		// serialised as "0"; fatal once they round-trip properly.
		//
		// HOST CONTROLS ARE NOT FILTERED HERE, and that is a deliberate
		// omission rather than an oversight. Tempo, song position and the two
		// halves of the time signature are stateful, so they pass this test and
		// go into every document: a SawDemo session file carries id="-2",
		// "-3", "-6" and "-7", all zero, which is measured and not assumed.
		// It is noise in a standalone's user-visible session.xml - there is no
		// transport to have produced a value - but adding the filter would take
		// them out of the getState chunk every DAW wrapper hands its host, and
		// that is a change to the shared format rather than a tidy-up. Both
		// RESET passes do skip them, so nothing applies a stale one over a
		// value the host has published.
		if (!parameter.info->is_stateful)
			continue;

		auto paramElement = doc.NewElement("Param");
		element->LinkEndChild(paramElement);
		paramElement->SetAttribute("id", handle);

		//const int voice = 0;
		//auto& raw = parameter.rawValues_[voice];

		//const auto val = RawToUtf8B(parameter.dataType, raw.data(), raw.size());

		// valueReal() is the DOUBLE accessor, so a non-scalar written through
		// it becomes "0" with its bytes silently discarded - which is why
		// neither a blob nor a string ever survived a preset round-trip.
		//
		// STRING IS BASE64 TOO, spelled exactly as a Blob is. That costs a
		// readable session.xml and buys three things an XML attribute cannot
		// otherwise give it, because a String parameter is a byte vector and not
		// text:
		//
		//   an embedded NUL used to TRUNCATE the value, because SetAttribute
		//   takes a C string - and XML 1.0 cannot represent U+0000 at all, so
		//   escaping could never have fixed that one, only a byte encoding can;
		//
		//   a control character used to be written RAW. tinyxml2 hands it
		//   straight back, so a round trip through this library looked lossless,
		//   but 0x01 is not a legal XML 1.0 character and attribute-value
		//   normalisation flattens a literal tab or newline to a space, so the
		//   document was one a conformant reader may reject or alter;
		//
		//   bytes that are not valid UTF-8 used to reach the host verbatim.
		//   AU2_Wrapper::SaveState puts this document through
		//   CFStringCreateWithCString, which answers NULL to exactly that and
		//   then passes the NULL to CFDictionaryAddValue.
		//
		// One encoding, in the one writer every wrapper shares, answers all
		// three - and makes `val` pure ASCII, so the last of them cannot come
		// back at some other call site. Nothing that ever worked is lost: until
		// the commit before this one a String went out as valueReal()'s "0", so
		// no file in existence holds a String value this could fail to read.
		// GmpiParameter::setFromXml is the other half - change neither alone.
		switch (parameter.info->datatype)
		{
		case gmpi::PinDatatype::String:
		case gmpi::PinDatatype::Blob:
		{
			if (const auto* v = std::get_if<std::vector<uint8_t>>(&parameter.value_); v)
				paramElement->SetAttribute("val", gmpi::base64Encode(*v).c_str());
			else
				paramElement->SetAttribute("val", "");
		}
		break;

		default:
			paramElement->SetAttribute("val", parameter.valueReal());
			break;
		}

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

std::string gmpi_controller_holder::getPresetXml() const
{
	return writePresetXml(patchManager.parameters);
}

void gmpi_controller_holder::notifyControllerOfPreset(gmpi::api::IParameterObserver* pluginController) const
{
	if (!pluginController)
		return;

	constexpr int32_t voice{};

	for (const auto& [handle, param] : patchManager.parameters)
	{
		if (!param.info || !param.info->is_stateful)
			continue;

		if (const auto* bytes = std::get_if<std::vector<uint8_t>>(&param.value_); bytes)
		{
			pluginController->setParameter(
				param.info->id, gmpi::Field::Value, voice,
				static_cast<int32_t>(bytes->size()), bytes->data());
		}
		else
		{
			const double value = param.valueReal();
			pluginController->setParameter(
				param.info->id, gmpi::Field::Value, voice,
				static_cast<int32_t>(sizeof(value)),
				reinterpret_cast<const uint8_t*>(&value));
		}
	}
}


} // namespace hosting
} // namespace gmpi

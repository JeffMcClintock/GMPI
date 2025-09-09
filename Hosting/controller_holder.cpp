#include "Hosting/controller_holder.h"

#include <string>
#include <vector>
#include <algorithm>

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
}

void gmpi_controller_holder::initUi(gmpi::api::IEditor* gui)
{
	m_editors.push_back(gui);

	for (auto& pin : info->guiPins)
	{
//		for (auto& param_info : info->parameters)
		{
//			if (param_info.id == pin.parameterId)
			{
				if (auto* param = patchManager.getParameter(pin.parameterId); param)
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
							const float value = static_cast<float>(param->valueReal);
							gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
//							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Int32:
						{
							const int32_t value = static_cast<int32_t>(std::round(param->valueReal));
							gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
//							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Bool:
						{
							const bool value = static_cast<bool>(std::round(param->valueReal));
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
							const float value = static_cast<float>(param->valueReal);
							gui->setParameter(param->info->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Int32:
						{
							const int32_t value = static_cast<int32_t>(std::round(param->valueReal));
							gui->setParameter(param->info->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
						}
						break;

						case gmpi::PinDatatype::Bool:
						{
							const bool value = static_cast<bool>(std::round(param->valueReal));
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
					const float value = static_cast<float>(param->valueReal);
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
					gui->notifyPin(pin.id, voice);
				}
				break;

				case gmpi::PinDatatype::Int32:
				{
					const int32_t value = static_cast<int32_t>(std::round(param->valueReal));
					gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
					gui->notifyPin(pin.id, voice);
				}
				break;

				case gmpi::PinDatatype::Bool:
				{
					const bool value = static_cast<bool>(std::round(param->valueReal));
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
							// updateProcessor
							pendingControllerQueueClients.AddWaiter(param);

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
														const float value = static_cast<float>(param->valueReal);
														gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
														//							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
													}
													break;

													case gmpi::PinDatatype::Int32:
													{
														const int32_t value = static_cast<int32_t>(std::round(param->valueReal));
														gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
														//							gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
													}
													break;

													case gmpi::PinDatatype::Bool:
													{
														const bool value = static_cast<bool>(std::round(param->valueReal));
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
                            const float value = static_cast<float>(param->valueReal);
                            gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
                            gui->notifyPin(pin.id, voice);
                            //                            gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(float), reinterpret_cast<const uint8_t*>(&value));
                        }
                        break;

                        case gmpi::PinDatatype::Int32:
                        {
                            const int32_t value = static_cast<int32_t>(std::round(param->valueReal));
                            gui->setPin(pin.id, 0, sizeof(value), reinterpret_cast<const uint8_t*>(&value));
                            gui->notifyPin(pin.id, voice);
                            //                            gui->setParameter(param->id, pin.parameterFieldType, 0, sizeof(int32_t), reinterpret_cast<const uint8_t*>(&value));
                        }
                        break;

                        case gmpi::PinDatatype::Bool:
                        {
                            const bool value = static_cast<bool>(std::round(param->valueReal));
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
	};

	return false; // failed to consume message.
}

} // namespace hosting
} // namespace gmpi

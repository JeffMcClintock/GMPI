/* Copyright (c) 2016 SynthEdit Ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name SynthEdit, nor SEM, nor 'Music Plugin Interface' nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY SYNTHEDIT LTD ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SYNTHEDIT LTD BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
#include "mp_interface_wrapper.h"
using namespace GmpiSdk;
*/

#pragma once

#include "mp_sdk_common.h"

namespace GmpiSdk
{
	// not for end-users.
	namespace Internal
	{
		template<class InterfaceClass>
		class GmpiIWrapper
		{
		protected:
			gmpi_sdk::mp_shared_ptr<InterfaceClass> m_ptr;

			GmpiIWrapper() {}
			GmpiIWrapper(GmpiIWrapper const & other) : m_ptr(other.m_ptr) {}
			GmpiIWrapper(GmpiIWrapper && other) : m_ptr(std::move(other.m_ptr)) {}
			void Copy(InterfaceClass* other) { m_ptr = other; }
			void Move(GmpiIWrapper && other) { m_ptr = std::move(other.m_ptr); }

		public:
			GmpiIWrapper(gmpi::IMpUnknown* /*other*/) : m_ptr(nullptr)
            {
            }

            inline InterfaceClass* Get()
			{
				return m_ptr.get();
			}

			inline gmpi::IMpUnknown*& Unknown() { return m_ptr.get(); };
			inline void** asIMpUnknownPtr(){ return m_ptr.asIMpUnknownPtr(); };
			inline bool isNull() { return m_ptr == nullptr; }
			void setNull() { m_ptr = nullptr; }
		};
	}
}

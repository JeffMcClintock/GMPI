#pragma once

// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.

#ifndef GMPI_REFCOUNT_MACROS_H_INCLUDED
#define GMPI_REFCOUNT_MACROS_H_INCLUDED

/*
#include "RefCountMacros.h"
*/

// macros to save typing the reference counting.
#define GMPI_QUERYINTERFACE( INTERFACE_CLASS ) \
	if ((*iid) == INTERFACE_CLASS::guid || (*iid) == gmpi::api::IUnknown::guid){ \
	*returnInterface = static_cast<INTERFACE_CLASS*>(this); addRef(); \
	return gmpi::ReturnCode::Ok;}

#define GMPI_QUERYINTERFACE_METHOD( INTERFACE_CLASS ) \
	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override{ \
	*returnInterface = {}; \
	GMPI_QUERYINTERFACE(INTERFACE_CLASS) \
	return gmpi::ReturnCode::NoSupport;}
#endif

#ifndef GMPI_REFCOUNT
#define GMPI_REFCOUNT int refCount2_ = 1; \
	int32_t addRef() override {return ++refCount2_;} \
	int32_t release() override {if (--refCount2_ == 0){delete this;return 0;}return refCount2_;}

#define GMPI_REFCOUNT_NO_DELETE \
	int32_t addRef() override{return 1;} \
	int32_t release() override {return 1;}
#endif

// help for preventing a source file being discarded by the linker
#undef SE_DECLARE_INIT_STATIC_FILE
#define SE_DECLARE_INIT_STATIC_FILE(filename) void se_static_library_init_##filename(){}

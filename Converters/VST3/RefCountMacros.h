#pragma once
//#include "GmpiSdkCommon.h" // for GUID equality operator

//	if ((*iid) == CLASS_NAME::guid || (*iid) == gmpi::api::IUnknown::guid ){ \
// 
// macros to save typing the reference counting.
#define GMPI_QUERYINTERFACE( CLASS_NAME ) \
	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override{ \
	*returnInterface = 0; \
	if (0 == std::memcmp(iid, &CLASS_NAME::guid, sizeof(gmpi::api::Guid)) || 0 == std::memcmp(iid, &gmpi::api::IUnknown::guid, sizeof(gmpi::api::Guid)) ){ \
	*returnInterface = static_cast<CLASS_NAME*>(this); addRef(); \
	return gmpi::ReturnCode::Ok;} \
	return gmpi::ReturnCode::NoSupport;}

#ifndef GMPI_REFCOUNT
#define GMPI_REFCOUNT int refCount2_ = 1; \
	int32_t addRef() override {return ++refCount2_;} \
	int32_t release() override {if (--refCount2_ == 0){delete this;return 0;}return refCount2_;}

#define GMPI_REFCOUNT_NO_DELETE \
	int32_t addRef() override{return 1;} \
	int32_t release() override {return 1;}
#endif

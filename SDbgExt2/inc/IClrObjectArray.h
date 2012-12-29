#pragma once
#include "IClrObject.h"

MIDL_INTERFACE("E46D45EC-36EF-4DE3-9077-B03F386F4CDF")
IClrObjectArray : public IUnknown
{
	virtual STDMETHODIMP GetItem(ULONG32 idx, IClrObject **ret) = 0;
};
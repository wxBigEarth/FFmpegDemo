#include "IO/IOHandle.h"


namespace avstudio
{
	int IIOHandle::WriteData(const AVMediaType n_eMediaType, 
		EDataType n_eType, void* n_Data)
	{
		return ReceiveData(n_eMediaType, n_eType, n_Data);
	}

	int IIOHandle::ReceiveData(const AVMediaType n_eMediaType, 
		EDataType n_eType, void* n_Data)
	{
		return 0;
	}

	size_t IIOHandle::GetBufferSize(const AVMediaType n_eMediaType) const
	{
		return 0;
	}

}
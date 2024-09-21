#ifndef __IOHANDLE_H__
#define __IOHANDLE_H__
#include "Util/Common.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	class IIOHandle
	{
	public:
		int WriteData(const AVMediaType n_eMediaType,
			EDataType n_eType, void* n_Data);

		virtual int ReceiveData(const AVMediaType n_eMediaType,
			EDataType n_eType, void* n_Data);

		virtual size_t GetBufferSize(const AVMediaType n_eMediaType) const;
	};
}



#endif // !__IOHANDLE_H__

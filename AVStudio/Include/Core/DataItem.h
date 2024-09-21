#ifndef __DATAITEM_H__
#define __DATAITEM_H__
#include "Util/Common.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	struct FDataItem
	{
		void*		Data = nullptr;
		AVMediaType	MediaType = AVMediaType::AVMEDIA_TYPE_UNKNOWN;
		EDataType	DataType = EDataType::DT_None;

		// Cover data to AVPacket*
		AVPacket*	p();
		// Cover data to AVFrame*
		AVFrame*	f();
	};

	// Free AVPacket/AVFrame by DataType
	void AVFreeData(FDataItem** n_DataItem);

}

#endif // !__DATAITEM_H__

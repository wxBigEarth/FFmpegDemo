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
	enum class EDataType
	{
		DT_None = 0,
		DT_Packet,
		DT_Frame,
	};

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

	// Clone AVFrame or AVPacket
	void* AVClone(EDataType n_eDataType, void* n_Data);

	// Free FDataItem and set n_DataItem to nullptr
	void AVFreeDataPtr(FDataItem** n_DataItem);

	// Free FDataItem
	void AVFreeData(FDataItem* n_DataItem);

	// Free AVFrame
	void AVFreeData(AVFrame** n_Frame);

	// Free AVPacket
	void AVFreeData(AVPacket** n_Packet);
}

#endif // !__DATAITEM_H__

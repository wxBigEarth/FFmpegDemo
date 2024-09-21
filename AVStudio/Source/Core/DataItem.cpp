#include "Core/DataItem.h"



namespace avstudio
{
	AVPacket* FDataItem::p()
	{
		return (AVPacket*)Data;
	}

	AVFrame* FDataItem::f()
	{
		return (AVFrame*)Data;
	}

	void AVFreeData(FDataItem** n_DataItem)
	{
		if (!n_DataItem || !(*n_DataItem)) return;

		FDataItem* DataItem = *n_DataItem;

		switch (DataItem->DataType)
		{
		case EDataType::DT_None:
			break;
		case EDataType::DT_Packet:
		{
			if (DataItem->Data) av_packet_free((AVPacket**)&DataItem->Data);
		}
		break;
		case EDataType::DT_Frame:
		{
			if (DataItem->Data) av_frame_free((AVFrame**)&DataItem->Data);
		}
		break;
		default:
			break;
		}

		delete *n_DataItem;
		*n_DataItem = nullptr;
	}

}

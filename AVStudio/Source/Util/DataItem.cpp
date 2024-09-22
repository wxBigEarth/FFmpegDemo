#include "Util/DataItem.h"



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

	void AVFreeDataPtr(FDataItem** n_DataItem)
	{
		if (!n_DataItem || !(*n_DataItem)) return;

		FDataItem* DataItem = *n_DataItem;

		AVFreeData(DataItem);

		delete *n_DataItem;
		*n_DataItem = nullptr;
	}

	void AVFreeData(FDataItem* n_DataItem)
	{
		switch (n_DataItem->DataType)
		{
		case EDataType::DT_None:
			break;
		case EDataType::DT_Packet:
		{
			if (n_DataItem->Data) av_packet_free((AVPacket**)&n_DataItem->Data);
		}
		break;
		case EDataType::DT_Frame:
		{
			if (n_DataItem->Data) av_frame_free((AVFrame**)&n_DataItem->Data);
		}
		break;
		default:
			break;
		}
	}

}

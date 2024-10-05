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

	void* AVClone(EDataType n_eDataType, void* n_Data)
	{
		void* Data = nullptr;

		if (!n_Data) return Data;

		if (n_eDataType == EDataType::DT_Frame)
		{
			AVFrame* Frame = av_frame_alloc();
			av_frame_move_ref(Frame, (AVFrame*)n_Data);
			Data = Frame;
		}
		else if (n_eDataType == EDataType::DT_Packet)
		{
			AVPacket* Packet = av_packet_alloc();
			av_packet_move_ref(Packet, (AVPacket*)n_Data);
			Data = Packet;
		}

		return Data;
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
			AVFreeData((AVPacket**)&n_DataItem->Data);
		}
		break;
		case EDataType::DT_Frame:
		{
			AVFreeData((AVFrame**)&n_DataItem->Data);
		}
		break;
		default:
			break;
		}
	}

	void AVFreeData(AVFrame** n_Frame)
	{
		if (n_Frame) av_frame_free(n_Frame);
	}

	void AVFreeData(AVPacket** n_Packet)
	{
		if (n_Packet) av_packet_free(n_Packet);
	}

}

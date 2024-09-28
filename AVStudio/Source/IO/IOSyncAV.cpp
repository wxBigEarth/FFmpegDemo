#include "IO/IOSyncAV.h"


namespace avstudio
{
	constexpr auto kTimeout = 10;

	static void FreeDataItem(FDataItem* n_Item)
	{
		AVFreeDataPtr(&n_Item);
	}

	CIOSyncAV::CIOSyncAV()
	{
	}

	CIOSyncAV::~CIOSyncAV()
	{
		Release();
	}

	int CIOSyncAV::ReceiveData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			FDataItem* DataItem = new FDataItem();
			DataItem->DataType = n_eDataType;
			DataItem->MediaType = n_eMediaType;
			DataItem->Data = n_Data;

			m_qVideo.Push(DataItem);
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			FDataItem* DataItem = new FDataItem();
			DataItem->DataType = n_eDataType;
			DataItem->MediaType = n_eMediaType;
			DataItem->Data = n_Data;

			m_qAudio.Push(DataItem);
		}

		return 0;
	}

	size_t CIOSyncAV::GetBufferSize(const AVMediaType n_eMediaType)
	{
		size_t nResult = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			nResult = m_qVideo.Size();
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			nResult = m_qAudio.Size();

		return nResult;
	}

	void CIOSyncAV::DataProcess()
	{
		if (IsAllStreamArrived()) Synchronize();
	}

	void CIOSyncAV::Synchronize()
	{
		int vRet = 0;
		int aRet = 0;
		int flag = 0;

		while (m_qVideo.Size() > 0 || m_qAudio.Size() > 0)
		{
			if (m_evStatus != EIOStatus::IO_Done && !m_vItem)
			{
				vRet = m_qVideo.Pop(m_vItem, kTimeout);
				if (vRet >= 0 && (!m_vItem || !m_vItem->Data))
					m_evStatus = EIOStatus::IO_Done;
			}

			if (m_eaStatus != EIOStatus::IO_Done && !m_aItem)
			{
				aRet = m_qAudio.Pop(m_aItem, kTimeout);
				if (aRet >= 0 && (!m_aItem || !m_aItem->Data))
					m_eaStatus = EIOStatus::IO_Done;
			}

			if (m_evStatus == EIOStatus::IO_Done || m_eaStatus == EIOStatus::IO_Done)
			{
				if (m_evStatus == EIOStatus::IO_Done)
				{
					AVFreeDataPtr(&m_vItem);
					ClearBuffer(AVMediaType::AVMEDIA_TYPE_VIDEO);

					if (m_eaStatus == EIOStatus::IO_Doing && m_aItem)
					{
						if (m_func) m_func(m_aItem);
						AVFreeDataPtr(&m_aItem);
					}
				}

				if (m_eaStatus == EIOStatus::IO_Done)
				{
					AVFreeDataPtr(&m_aItem);
					ClearBuffer(AVMediaType::AVMEDIA_TYPE_AUDIO);

					if (m_evStatus == EIOStatus::IO_Doing && m_vItem)
					{
						if (m_func) m_func(m_vItem);
						AVFreeDataPtr(&m_vItem);
					}
				}
			}
			else if (m_vItem && m_aItem)
			{
				if (m_vItem->DataType == EDataType::DT_Frame)
				{
					AVFrame* v = m_vItem->f();
					AVFrame* a = m_aItem->f();

					flag = av_compare_ts(v->pts, v->time_base,
						a->pts, a->time_base);
				}
				else if (m_vItem->DataType == EDataType::DT_Packet)
				{
					AVPacket* v = m_vItem->p();
					AVPacket* a = m_aItem->p();

					flag = av_compare_ts(v->pts, v->time_base,
						a->pts, a->time_base);
				}

				if (flag < 0)
				{
					if (m_func) m_func(m_vItem);
					AVFreeDataPtr(&m_vItem);
				}
				else
				{
					if (m_func) m_func(m_aItem);
					AVFreeDataPtr(&m_aItem);
				}
			}
			else
			{
				break;
			}
		}
	}

	void CIOSyncAV::Release()
	{
		ClearBuffer(AVMediaType::AVMEDIA_TYPE_VIDEO);
		ClearBuffer(AVMediaType::AVMEDIA_TYPE_AUDIO);

		m_vItem = nullptr;
		m_aItem = nullptr;

		IIOHandle::Release();
	}

	void CIOSyncAV::ClearBuffer(const AVMediaType n_eMediaType)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			m_qVideo.Clear(std::bind(&FreeDataItem, std::placeholders::_1));
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			m_qAudio.Clear(std::bind(&FreeDataItem, std::placeholders::_1));
	}

}
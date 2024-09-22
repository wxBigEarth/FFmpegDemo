#include "IO/IOHandle.h"


namespace avstudio
{
	IIOHandle::IIOHandle()
	{
		// Start with nullptr element
		// nullptr data indicates end
		WriteData(AVMediaType::AVMEDIA_TYPE_VIDEO, EDataType::DT_None, nullptr);
		WriteData(AVMediaType::AVMEDIA_TYPE_AUDIO, EDataType::DT_None, nullptr);

		m_itrVideo = m_lstVideo.begin();
		m_itrAudio = m_lstAudio.begin();
	}

	IIOHandle::~IIOHandle()
	{
		Release();
	}

	void IIOHandle::Release()
	{
		while (m_itrVideo != m_lstVideo.end())
		{
			AVFreeDataPtr(&(*m_itrVideo));

			m_lstVideo.erase(m_itrVideo);
			m_itrVideo = m_lstVideo.begin();
		}

		while (m_itrAudio != m_lstAudio.end())
		{
			AVFreeDataPtr(&(*m_itrAudio));

			m_lstAudio.erase(m_itrAudio);
			m_itrAudio = m_lstAudio.begin();
		}
	}

	int IIOHandle::WriteData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			FDataItem* DataItem = new FDataItem();
			DataItem->DataType = n_eDataType;
			DataItem->MediaType = n_eMediaType;
			DataItem->Data = n_Data;

			m_lstVideo.push_back(DataItem);
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			FDataItem* DataItem = new FDataItem();
			DataItem->DataType = n_eDataType;
			DataItem->MediaType = n_eMediaType;
			DataItem->Data = n_Data;

			m_lstAudio.push_back(DataItem);
		}

		return ReceiveData();
	}

	int IIOHandle::ReceiveData()
	{
		return 0;
	}

	size_t IIOHandle::GetBufferSize(const AVMediaType n_eMediaType)
	{
		size_t nResult = 0;

		std::unique_lock<std::mutex> lock(_mutex);

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			nResult = m_lstVideo.size();
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			nResult = m_lstAudio.size();

		return nResult;
	}

	bool IIOHandle::FetchData(std::function<void(FDataItem*)> n_func)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		auto vEnd = IsEnd(AVMediaType::AVMEDIA_TYPE_VIDEO);
		auto aEnd = IsEnd(AVMediaType::AVMEDIA_TYPE_AUDIO);

		if (vEnd == AVERROR_EOF || aEnd == AVERROR_EOF)
		{
			if (vEnd == AVERROR_EOF)
			{
				while (m_itrAudio != m_lstAudio.end() && 
					(*m_itrAudio)->Data)
				{
					ApplyData(AVMediaType::AVMEDIA_TYPE_AUDIO, n_func);
				}
			}

			if (aEnd == AVERROR_EOF)
			{
				while (m_itrVideo != m_lstVideo.end() &&
					(*m_itrVideo)->Data)
				{
					ApplyData(AVMediaType::AVMEDIA_TYPE_VIDEO, n_func);
				}
			}
		}
		else
		{
			while (m_itrVideo != m_lstVideo.end() && 
				(*m_itrVideo)->Data && 
				m_itrAudio != m_lstAudio.end() &&
				(*m_itrAudio)->Data)
			{
				AVPacket* v = (*m_itrVideo)->p();
				AVPacket* a = (*m_itrAudio)->p();

				if (av_compare_ts(v->pts, v->time_base,
					a->pts, a->time_base) < 0)
				{
					ApplyData(AVMediaType::AVMEDIA_TYPE_VIDEO, n_func);
				}
				else
				{
					ApplyData(AVMediaType::AVMEDIA_TYPE_AUDIO, n_func);
				}
			}

		}

		return IsOver();
	}

	bool IIOHandle::IsOver()
	{
		auto vEnd = IsEnd(AVMediaType::AVMEDIA_TYPE_VIDEO);
		auto aEnd = IsEnd(AVMediaType::AVMEDIA_TYPE_AUDIO);

		return vEnd == AVERROR_EOF && aEnd == AVERROR_EOF;
	}

	int IIOHandle::IsEnd(const AVMediaType n_eMediaType)
	{
		int nRet = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (m_itrVideo == m_lstVideo.end())
				m_itrVideo = m_lstVideo.begin();

			if (m_itrVideo == m_lstVideo.end())
				nRet = 0;
			else if ((*m_itrVideo)->Data)
				nRet = 1;
			else
			{
				std::list<FDataItem*>::iterator itr = m_itrVideo;
				itr++;

				while (itr != m_lstVideo.end() && !(*itr)->Data)
				{
					// Remove nullptr item until the last one
					ApplyData(AVMediaType::AVMEDIA_TYPE_VIDEO, nullptr);
					itr = m_itrVideo;
					itr++;
				}

				if (itr == m_lstVideo.end()) nRet = AVERROR_EOF;
				else
				{
					nRet = 1;
					ApplyData(AVMediaType::AVMEDIA_TYPE_VIDEO, nullptr);
				}
			}
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (m_itrAudio == m_lstAudio.end())
				m_itrAudio = m_lstAudio.begin();

			if (m_itrAudio == m_lstAudio.end())
				nRet = 0;
			else if ((*m_itrAudio)->Data)
				nRet = 1;
			else
			{
				std::list<FDataItem*>::iterator itr = m_itrAudio;
				itr++;

				while (itr != m_lstAudio.end() && !(*itr)->Data)
				{
					// Remove nullptr item until the last one
					ApplyData(AVMediaType::AVMEDIA_TYPE_AUDIO, nullptr);
					itr = m_itrAudio;
					itr++;
				}

				if (itr == m_lstAudio.end()) nRet = AVERROR_EOF;
				else
				{
					nRet = 1;
					ApplyData(AVMediaType::AVMEDIA_TYPE_AUDIO, nullptr);
				}
			}
		}

		return nRet;
	}

	void IIOHandle::ApplyData(const AVMediaType n_eMediaType,
		std::function<void(FDataItem*)> n_func)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			std::list<FDataItem*>::iterator itr = m_itrVideo++;
			if (n_func) n_func(*itr);

			AVFreeDataPtr(&(*itr));
			m_lstVideo.erase(itr);
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			std::list<FDataItem*>::iterator itr = m_itrAudio++;
			if (n_func) n_func(*itr);

			AVFreeDataPtr(&(*itr));
			m_lstAudio.erase(itr);
		}
	}

}
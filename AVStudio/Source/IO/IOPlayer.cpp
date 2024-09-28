#include "IO/IOPlayer.h"

namespace avstudio
{
	// delay 10 milliseconds
	constexpr auto kDelay = 10;

	CIOPlayer::~CIOPlayer()
	{

	}

	int CIOPlayer::WriteData(const AVMediaType n_eMediaType, 
		EDataType n_eDataType, void* n_Data, const int n_nSize /*= 0*/)
	{
		if (n_eDataType != EDataType::DT_Frame)
			return 0;

		return IIOHandle::WriteData(n_eMediaType, n_eDataType, n_Data, 0);
	}

	int CIOPlayer::ReceiveData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			m_lstVideo.push_back((AVFrame*)n_Data);
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			m_lstAudio.push_back((AVFrame*)n_Data);

		return 0;
	}

	size_t CIOPlayer::GetBufferSize(const AVMediaType n_eMediaType)
	{
		size_t nResult = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			nResult = m_lstVideo.size();
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			nResult = m_lstAudio.size();

		return nResult;
	}

	void CIOPlayer::Processing()
	{
		IIOHandle::Processing();

		Join();

		if (m_evStatus == EIOStatus::IO_Wait)
			m_tVideo = std::thread(std::bind(&CIOPlayer::PlayVideo, this));

		//if (m_eaStatus == EIOStatus::IO_Wait)
		//	m_tAudio = std::thread(std::bind(&CIOPlayer::PlayAudio, this));

		m_tEvent = std::thread(std::bind(&CIOPlayer::PlayerEvent, this));
	}

	void CIOPlayer::Release()
	{
		IIOHandle::Release();

		Join();

		auto vitr = m_lstVideo.begin();
		while (vitr != m_lstVideo.end())
		{
			av_frame_free(&(*vitr));
			m_lstVideo.erase(vitr++);
		}

		auto aitr = m_lstAudio.begin();
		while (aitr != m_lstAudio.end())
		{
			av_frame_free(&(*aitr));
			m_lstAudio.erase(aitr++);
		}

		m_dPlayTime = 0;
	}

	int CIOPlayer::PopAudio(AVFrame*& n_Frame)
	{
		n_Frame = nullptr;
		if (m_lstAudio.size() == 0) return -1;

		n_Frame = m_lstAudio.front();
		m_lstAudio.pop_front();

		if (n_Frame)
			m_dPlayTime = n_Frame->pts * av_q2d(n_Frame->time_base);
		else
			m_eaStatus = EIOStatus::IO_Done;

		return 0;
	}

	void CIOPlayer::Join()
	{
		if (m_tVideo.joinable()) m_tVideo.join();
		if (m_tAudio.joinable()) m_tAudio.join();
		if (m_tEvent.joinable()) m_tEvent.join();
	}

	void CIOPlayer::PlayVideo()
	{
		// Both video and Audio data should be arrived
		while (!IsAllStreamArrived())
			std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

		AVFrame* Frame = nullptr;
		double dTimestamp = 0;

		while (m_evStatus == EIOStatus::IO_Doing)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

			if (!Frame)
			{
				std::unique_lock<std::mutex> lock(m_mutex);

				if (m_lstVideo.size() == 0) continue;

				Frame = m_lstVideo.front();
				m_lstVideo.pop_front();

				if (!Frame)
				{
					m_evStatus = EIOStatus::IO_Done;
					break;
				}

				dTimestamp = Frame->pts * av_q2d(Frame->time_base);
			}

			if (dTimestamp > m_dPlayTime) continue;

			UpdateVideo(Frame, dTimestamp);
			av_frame_free(&Frame);
		}
	}

	void CIOPlayer::PlayAudio()
	{
		// Both video and Audio data should be arrived
		while (!IsAllStreamArrived())
			std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

		AVFrame* Frame = nullptr;
		double dTimestamp = 0;

		while (m_eaStatus == EIOStatus::IO_Doing)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

			if (!Frame)
			{
				std::unique_lock<std::mutex> lock(m_mutex);

				if (m_lstAudio.size() == 0) continue;

				Frame = m_lstAudio.front();
				m_lstAudio.pop_front();

				if (!Frame)
				{
					m_eaStatus = EIOStatus::IO_Done;
					break;
				}

				dTimestamp = Frame->pts * av_q2d(Frame->time_base);
			}

			if (dTimestamp > m_dPlayTime) continue;

			UpdateAudio(Frame, dTimestamp);
			av_frame_free(&Frame);
		}
	}

	void CIOPlayer::PlayerEvent()
	{
		// Both video and Audio data should be arrived
		while (!IsAllStreamArrived())
			std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

		while (m_eaStatus != EIOStatus::IO_Done ||
			m_evStatus != EIOStatus::IO_Done)
		{
			UpdateEvent();
			std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));
		}
	}

	void CIOPlayer::UpdateTime()
	{

	}

	void CIOPlayer::UpdateEvent()
	{

	}

}
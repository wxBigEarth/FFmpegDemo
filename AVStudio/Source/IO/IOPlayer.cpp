#include "IO/IOPlayer.h"
#include "Util/Debug.h"
#if defined(_WIN32) || defined(_WIN64) 

#elif defined(__unix__) || defined(__unix)
#include <time.h>
#endif

namespace avstudio
{
	// delay 3 milliseconds
	constexpr auto kDelay = 3;

	CIOPlayer::~CIOPlayer()
	{
		Release2();
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
		Release2();

		IIOHandle::Processing();

		Join();

		if (m_evStatus == EIOStatus::IO_Wait)
			m_tPlay = std::thread(std::bind(&CIOPlayer::PlayProc, this));
	}

	void CIOPlayer::Release()
	{

	}

	void CIOPlayer::Release2()
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

		m_dAudioTime = 0;
		m_dAudioQ = 0;
		m_dVideoTime = 0;
		m_dVideoQ = 0;

		if (m_vFrame) av_frame_free(&m_vFrame);
		if (m_aFrame) av_frame_free(&m_aFrame);
	}

	void CIOPlayer::SetupCallback(FCallback<void, EIOPEventId> n_cb)
	{
		m_PlayerCb = n_cb;
	}

	int CIOPlayer::PopVideo(AVFrame*& n_Frame)
	{
		n_Frame = nullptr;
		m_dVideoTime = AVERROR(EAGAIN);

		// Maybe it will force stop by user
		if (m_evStatus == EIOStatus::IO_Done) return AVERROR_EOF;

		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_lstVideo.size() == 0) return AVERROR(EAGAIN);

		auto Frame = m_lstVideo.front();
		m_lstVideo.pop_front();

		m_vFrame = (AVFrame*)AVCloneRef(EDataType::DT_Frame, Frame, m_vFrame);
		AVFreeData(&Frame);
		n_Frame = m_vFrame;

		if (m_dVideoQ == 0)
			m_dVideoQ = av_q2d(n_Frame->time_base);
		if (n_Frame)
			m_dVideoTime = n_Frame->pts * m_dVideoQ;
		else
		{
			m_evStatus = EIOStatus::IO_Done;
			if (IsAllStreamDone()) PlayEnd();
		}

		return 0;
	}

	int CIOPlayer::PopAudio(AVFrame*& n_Frame)
	{
		n_Frame = nullptr;

		// Maybe it will force stop by user
		if (m_eaStatus == EIOStatus::IO_Done) return AVERROR_EOF;

		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_lstAudio.size() == 0) return AVERROR(EAGAIN);

		auto Frame = m_lstAudio.front();
		m_lstAudio.pop_front();

		m_aFrame = (AVFrame*)AVCloneRef(EDataType::DT_Frame, Frame, m_aFrame);
		AVFreeData(&Frame);
		n_Frame = m_aFrame;

		if (m_dAudioQ == 0)
			m_dAudioQ = av_q2d(n_Frame->time_base);
		if (n_Frame)
			m_dAudioTime = n_Frame->pts * m_dAudioQ;
		else
		{
			m_eaStatus = EIOStatus::IO_Done;
			if (IsAllStreamDone()) PlayEnd();
		}

		return 0;
	}

	const double CIOPlayer::PlayedTime() const
	{
		return m_dAudioTime;
	}

	void CIOPlayer::Join()
	{
		if (m_tPlay.joinable()) m_tPlay.join();
	}

	void CIOPlayer::PlayProc()
	{
		// Both video and Audio data should be arrived
		while (!IsAllStreamArrived())
			std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

		while (m_evStatus == EIOStatus::IO_Doing)
		{
			if (m_dVideoTime != AVERROR(EAGAIN))
			{
				auto delta = m_dVideoTime - m_dAudioTime;
				if (delta > 0)
				{
#if defined(_WIN32) || defined(_WIN64) 
					// On Windows, sleep accuracy is insufficient, 
					// reduce 1 millisecond for correction
					int t = (int)(delta * 1000) - 1;
					std::this_thread::sleep_for(std::chrono::milliseconds(t));
#elif defined(__unix__) || defined(__unix)
					struct timespec ts;
					ts.tv_sec = (int)delta;
					ts.tv_nsec = (delta - ts.tv_sec) * 1000000000;
					nanosleep(&ts, NULL);
#endif
				}

				if (m_eaStatus == EIOStatus::IO_Done)
				{
					// If audio stream is end, update time
					m_dAudioTime = m_dVideoTime;
				}

				m_dVideoTime = AVERROR(EAGAIN);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));
			}

			Update();
		}
	}

	int CIOPlayer::Update()
	{
		m_PlayerCb.Execute(EIOPEventId::PEI_UpdateVideo);
		return 0;
	}

	void CIOPlayer::PlayEnd()
	{
		m_PlayerCb.Execute(EIOPEventId::PEI_EOF);
	}

}
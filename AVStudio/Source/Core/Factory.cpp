#include "Core/Factory.h"
#include "Apis/Frame.h"
#include "Util/Debug.h"


namespace avstudio
{
	CFactory::CFactory(std::shared_ptr<FWorkShop> n_Output)
	{
		m_Output = n_Output;
	}

	CFactory::~CFactory()
	{
		Release();
	}

	std::shared_ptr<FWorkShop> CFactory::Input()
	{
		if (!m_Input) m_Input = std::make_shared<FWorkShop>();
		return m_Input;
	}

	int CFactory::Processing(const bool n_bIsLastItem, const bool n_bIsLastGroup)
	{
		m_Input->Processing();
		m_Input->OpenCodecContext();
		m_Output->Processing();

		m_bIsLastItem = n_bIsLastItem;
		m_bIsLastGroup = n_bIsLastGroup;

		return 0;
	}

	int CFactory::WindUp()
	{
		return 0;
	}

	int CFactory::Editing()
	{
		if (m_bIsEnd) return AVERROR_EOF;
		if (!m_Input) return AVERROR_EOF;
		// For recording, input context is invalid
		if (!m_Input->IsValid())
		{
			std::this_thread::sleep_for(std::chrono::microseconds(10));
			return 0;
		}
		if (!m_Packet) m_Packet = av_packet_alloc();

		int ret = ReadPacket(m_Packet);
		if (ret < 0)
		{
			Flush();
			return AVERROR_EOF;
		}

		auto eMediaType = m_Input->Fmt.GetStreamMediaType(m_Packet->stream_index);
		auto nOffset = m_Input->TryPickup(m_Packet->pts, eMediaType);
		if (nOffset == AVERROR_EOF)
		{
			Flush();
			return AVERROR_EOF;
		}

		//if (m_Packet->stream_index == m_Input->VideoParts.nStreamIndex)
		//{
		//	auto t = m_Packet->pts * av_q2d(m_Input->VideoParts.Stream->time_base);
		//	AVDebug("Video : %lf\n", t);
		//}
		//else if (m_Packet->stream_index == m_Input->AudioParts.nStreamIndex)
		//{
		//	auto t = m_Packet->pts * av_q2d(m_Input->AudioParts.Stream->time_base);
		//	AVDebug("Audio : %lf\n", t);
		//}

		if (nOffset >= 0) Demuxing(m_Packet, eMediaType);

		av_packet_unref(m_Packet);

		return ret;
	}

	int CFactory::WriteFrame(AVFrame* n_Frame, AVMediaType n_eMediaType)
	{
		return Converting(n_Frame, n_eMediaType);
	}

	void CFactory::Flush()
	{
		if (m_Input->IsValid())
		{
			if (!m_Packet) m_Packet = av_packet_alloc();

			m_Packet->data = nullptr;
			m_Packet->size = 0;

			if (m_Input->VideoParts.Stream)
			{
				m_Packet->stream_index = m_Input->VideoParts.nStreamIndex;
				Decoding(m_Packet, AVMediaType::AVMEDIA_TYPE_VIDEO);
			}

			if (m_Input->AudioParts.Stream)
			{
				m_Packet->stream_index = m_Input->AudioParts.nStreamIndex;
				Decoding(m_Packet, AVMediaType::AVMEDIA_TYPE_AUDIO);
			}
		}
		else
		{
			// For writing PCM data
			if (m_Input->VideoParts.Codec)
				WriteFrame(nullptr, AVMediaType::AVMEDIA_TYPE_VIDEO);
			if (m_Input->AudioParts.Codec)
				WriteFrame(nullptr, AVMediaType::AVMEDIA_TYPE_AUDIO);
		}

		m_bIsEnd = true;
	}

	void CFactory::SetIoHandle(std::shared_ptr<IIOHandle> n_Handle)
	{
		m_IoHandle = n_Handle;
	}

	const bool CFactory::IsEnd() const
	{
		return m_bIsEnd;
	}

	void CFactory::Release()
	{
		m_bIsLastItem = false;
		m_bIsLastGroup = false;

		if (m_Packet) av_packet_free(&m_Packet);
		if (m_vFrame) av_frame_free(&m_vFrame);
		if (m_aFrame) av_frame_free(&m_aFrame);
	}

	int CFactory::ReadPacket(AVPacket* n_Packet)
	{
		return m_Input->Fmt.ReadPacket(n_Packet);
	}

	int CFactory::Demuxing(AVPacket* n_Packet, AVMediaType n_eMediaType)
	{
		if (!m_Input->CheckMedia(n_eMediaType)) return 0;

		FLatheParts* oLatheParts = nullptr;
		FLatheParts* iLatheParts = nullptr;

		// For video stream, default index is 0
		// For Audio stream, default index is 1
		if (n_Packet->stream_index == m_Input->VideoParts.nStreamIndex)
		{
			oLatheParts = &m_Output->VideoParts;
			iLatheParts = &m_Input->VideoParts;

			n_Packet->stream_index = 0;
			n_Packet->time_base = iLatheParts->Stream->time_base;
		}
		else if (n_Packet->stream_index == m_Input->AudioParts.nStreamIndex)
		{
			oLatheParts = &m_Output->AudioParts;
			iLatheParts = &m_Input->AudioParts;

			// If no video stream, audio stream index is 0
			n_Packet->stream_index = iLatheParts->nStreamIndex < 0 ? 0 : 1;
			n_Packet->time_base = iLatheParts->Stream->time_base;
		}

		n_Packet->dts = iLatheParts->PacketPts + n_Packet->pts - n_Packet->dts;
		n_Packet->pts = iLatheParts->PacketPts;
		iLatheParts->PacketPts += n_Packet->duration;

		if (iLatheParts->nFlag == 0 && m_Output->IsValid()
			&& av_cmp_q(iLatheParts->Stream->time_base,
				oLatheParts->Stream->time_base) != 0)
		{
			// If no need to decode, and output context is valid, change timebase
			av_packet_rescale_ts(n_Packet,
				iLatheParts->Stream->time_base,
				oLatheParts->Stream->time_base);

			n_Packet->time_base = oLatheParts->Stream->time_base;
		}

		return Decoding(n_Packet, n_eMediaType);
	}

	int CFactory::Decoding(AVPacket* n_Packet, AVMediaType n_eMediaType)
	{
		FLatheParts* oLatheParts = nullptr;
		FLatheParts* iLatheParts = nullptr;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			oLatheParts = &m_Output->VideoParts;
			iLatheParts = &m_Input->VideoParts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			oLatheParts = &m_Output->AudioParts;
			iLatheParts = &m_Input->AudioParts;
		}

		if (!oLatheParts || !iLatheParts) return 0;
		if (iLatheParts->nFlag == 0)
		{
			AVPacket* p = n_Packet;
			if (!p || !n_Packet->data || n_Packet->size == 0)
				p = nullptr;
			else
				p->pts += m_Output->GetLastPts(n_eMediaType);

			PushData(n_eMediaType, EDataType::DT_Packet, p);
			return 0;
		}

		int ret = iLatheParts->Codec->DecodePacket(n_Packet,
			[this, n_eMediaType, iLatheParts, oLatheParts](AVFrame* n_Frame) {

				if (n_Frame)
				{
					//n_Frame->pkt_dts = n_Frame->pts;
					n_Frame->pts = n_Frame->best_effort_timestamp;
					if (n_Frame->pict_type == AVPictureType::AV_PICTURE_TYPE_B &&
						oLatheParts->Codec && 
						oLatheParts->Codec->Context &&
						oLatheParts->Codec->Context->has_b_frames == 0)
					{
						// If output codec do not support b frame, change the picture type
						n_Frame->pict_type = AVPictureType::AV_PICTURE_TYPE_P;
					}

					if (iLatheParts->Codec->Context->codec_id == AVCodecID::AV_CODEC_ID_MP3 &&
						m_bFirstMp3Frame)
					{
						m_bFirstMp3Frame = false;
						if (n_Frame->nb_samples < iLatheParts->Codec->Context->frame_size)
							return 0;
					}

					n_Frame->time_base = iLatheParts->Codec->Context->time_base;
					n_Frame->duration = iLatheParts->Duration;
				}

				return Converting(n_Frame, n_eMediaType);
			});

		return ret;
	}

	int CFactory::Converting(AVFrame* n_Frame, AVMediaType n_eMediaType)
	{
		int ret = 0;
		FLatheParts* LatheParts = nullptr;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			AVFrame* Frame = n_Frame;
			LatheParts = &m_Input->VideoParts;

			if (n_Frame && 
				LatheParts->Sws &&
				LatheParts->Sws->Context)
			{
				m_vFrame = FFrame::VideoFrame(
					LatheParts->Sws->GetOutputWidth(),
					LatheParts->Sws->GetOutputHeight(),
					LatheParts->Sws->GetOutputPixelFormat(),
					m_vFrame);

				LatheParts->Sws->Scale(n_Frame->data, n_Frame->linesize,
					m_vFrame->data, m_vFrame->linesize);

				m_vFrame->pts = n_Frame->pts;
				m_vFrame->duration = n_Frame->duration;
				m_vFrame->time_base = n_Frame->time_base;
				Frame = m_vFrame;
				//LogInfo("Video frame: %zd.\n", Frame->pts);
			}
			
			if (Frame)
			{
				Frame->pts = m_Input->AdjustPts(Frame->duration, n_eMediaType);

				if (m_Output->VideoParts.Codec && m_Output->VideoParts.Codec->Context)
				{
					Frame->time_base = m_Output->VideoParts.Codec->Context->time_base;
					auto from = LatheParts->Codec->Context->time_base;
					auto to = Frame->time_base;

					int cmp = av_cmp_q(from, to);
					if (cmp != 0 && cmp != INT_MIN)
						Frame->pts = av_rescale_q(Frame->pts, from, to);
				}

				Frame->pts += m_Output->GetLastPts(n_eMediaType);
			}

			ret = Filtering(Frame, n_eMediaType);
			if (Frame == m_vFrame) av_frame_unref(Frame);
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			LatheParts = &m_Input->AudioParts;

			if (!n_Frame)
			{
				while (m_bIsLastItem && 
					LatheParts->FiFo && LatheParts->FiFo->Size() > 0)
					PopFromFifo();

				ret = Filtering(n_Frame, n_eMediaType);
			}
			else if (LatheParts->FiFo && LatheParts->FiFo->Context)
			{
				if (LatheParts->Resample && LatheParts->Resample->Context)
				{
					int nSamples = LatheParts->Resample->Cover(n_Frame);
					LatheParts->FiFo->Push(LatheParts->Resample->CoverData, nSamples);
				}
				else
				{
					LatheParts->FiFo->Push(n_Frame->data, n_Frame->nb_samples);
				}

				while (LatheParts->FiFo->IsReadable())
					ret = PopFromFifo();
			}
			else if (LatheParts->Resample)
			{
				AVFrame* Frame = AllocAudioFrame(
					m_Output->AudioParts.Codec->Context->frame_size);

				LatheParts->Resample->Cover(
					(const uint8_t**)n_Frame->extended_data,
					n_Frame->nb_samples,
					Frame->data,
					Frame->nb_samples);

				ret = Filtering(Frame, n_eMediaType);
				av_frame_unref(Frame);
			}
			else
			{
				n_Frame->pts = m_Input->AdjustPts(n_Frame->nb_samples, n_eMediaType) +
					m_Output->GetLastPts(n_eMediaType);

				ret = Filtering(n_Frame, n_eMediaType);
			}
		}

		return ret;
	}

	int CFactory::Filtering(AVFrame* n_Frame, AVMediaType n_eMediaType)
	{
		int ret = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO &&
			m_Input->VideoParts.Filter)
		{
			auto& Filter = m_Input->VideoParts.Filter;
			Filter->Push(m_Input->GetGroupIndex(), n_Frame);

			ret = Filter->Pop(
				[this, n_eMediaType](AVFrame* Frame) {
					return Encoding(Frame, n_eMediaType);
				});
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO &&
			m_Input->AudioParts.Filter)
		{
			auto& Filter = m_Input->AudioParts.Filter;
			Filter->Push(m_Input->GetGroupIndex(), n_Frame);

			ret = Filter->Pop(
				[this, n_eMediaType](AVFrame* Frame) {
					return Encoding(Frame, n_eMediaType);
				});
		}
		else
		{
			ret = Encoding(n_Frame, n_eMediaType);
		}

		return ret;
	}

	int CFactory::Encoding(AVFrame* n_Frame, AVMediaType n_eMediaType)
	{
		int ret = 0;

		FLatheParts* oLatheParts = nullptr;
		FLatheParts* iLatheParts = nullptr;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			oLatheParts = &m_Output->VideoParts;
			iLatheParts = &m_Input->VideoParts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			oLatheParts = &m_Output->AudioParts;
			iLatheParts = &m_Input->AudioParts;
		}

		if (!oLatheParts || !iLatheParts) return ret;

		if (!oLatheParts->Codec->IsOpen() ||
			!oLatheParts->Stream || 
			(!n_Frame && (!m_bIsLastItem || !m_bIsLastGroup)) ||
			!m_Output->IsValid())
		{
			PushData(n_eMediaType, EDataType::DT_Frame, n_Frame);
			return ret;
		}

		if (n_Frame && n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			// If not do this, it may trigger warning: 
			// forced frame type (3) at 12 was changed to frame type (2)
			n_Frame->pict_type = AVPictureType::AV_PICTURE_TYPE_NONE;
		}

		ret = oLatheParts->Codec->EncodeFrame(n_Frame,
			[this, n_eMediaType, oLatheParts](AVPacket* n_Packet) {

				if (n_Packet)
				{
					auto from = oLatheParts->Codec->Context->time_base;
					auto to = oLatheParts->Stream->time_base;

					int cmp = av_cmp_q(from, to);
					if (cmp != 0 && cmp != INT_MIN)
						av_packet_rescale_ts(n_Packet, from, to);
					n_Packet->time_base = to;
					n_Packet->stream_index = oLatheParts->nStreamIndex;
				}

				PushData(n_eMediaType, EDataType::DT_Packet, n_Packet);

				return 0;
			});

		return ret;
	}

	void CFactory::PushData(AVMediaType n_eMediaType, 
		EDataType n_eDataType, void* n_Data)
	{
		if (!m_IoHandle) return;

		m_IoHandle->WriteData(n_eMediaType, n_eDataType, n_Data);
		m_IoHandle->DataProcess();
	}

	int CFactory::PopFromFifo()
	{
		int nSampleSize = m_Input->AudioParts.FiFo->NextSampleCount();
		if (nSampleSize <= 0) return -1;

		AVFrame* Frame = AllocAudioFrame(nSampleSize);

		m_Input->AudioParts.FiFo->Pop(Frame);

		if (nSampleSize < Frame->nb_samples)
		{
			// For the last frame, it should be filled with silence data
			av_samples_set_silence(Frame->data, nSampleSize,
				Frame->nb_samples - nSampleSize,
				Frame->ch_layout.nb_channels,
				(AVSampleFormat)Frame->format);
		}

		int ret = Filtering(Frame, AVMediaType::AVMEDIA_TYPE_AUDIO);

		av_frame_unref(Frame);

		return ret;
	}

	AVFrame* CFactory::AllocAudioFrame(int n_nFrameSize)
	{
		auto ctx = m_Output->AudioParts.Codec->Context;

		int nFrameSize = n_nFrameSize;

		if (nFrameSize < ctx->frame_size &&
			(!m_bIsLastGroup || m_Input->AudioParts.Filter))
		{
			// For the last frame, if current group is not last group
			// remain samples will be filled with silence samples
			nFrameSize = ctx->frame_size;
		}

		m_aFrame = FFrame::AudioFrame(
			nFrameSize,
			ctx->sample_rate,
			ctx->sample_fmt,
			&ctx->ch_layout,
			m_aFrame);

		m_aFrame->pts = m_Input->AdjustPts(m_aFrame->nb_samples,
			AVMediaType::AVMEDIA_TYPE_AUDIO) +
			m_Output->GetLastPts(AVMediaType::AVMEDIA_TYPE_AUDIO);
		m_aFrame->time_base = ctx->time_base;

		//LogInfo("Audio frame: %zd.\n", Frame->pts);

		return m_aFrame;
	}

}

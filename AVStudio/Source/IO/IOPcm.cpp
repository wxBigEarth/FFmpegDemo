#include "IO/IOPcm.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/pixdesc.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	void CIOPcm::SetupInputParameter(AVCodecContext* n_CodecCtx)
	{
		if (n_CodecCtx->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
			m_vCodecCtx = n_CodecCtx;
		else if (n_CodecCtx->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
			m_aCodecCtx = n_CodecCtx;
	}

	int CIOPcm::WriteData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data, const int n_nSize)
	{
		int ret = -1;
		AVFrame* Frame = nullptr;

		if (m_vCodecCtx &&
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			m_vFrame = FFrame::VideoFrame(
				m_vCodecCtx->width,
				m_vCodecCtx->height, 
				m_vCodecCtx->pix_fmt, 
				m_vFrame);

			if (m_vFrame)
			{
				FillVideoFrame(m_vFrame, n_Data, n_nSize);
				m_vFrame->duration = 1;
				m_vFrame->pts = m_nvFrameIndex++;
				Frame = m_vFrame;
			}
		}

		if (m_aCodecCtx &&
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			m_aFrame = FFrame::AudioFrame(
				m_aCodecCtx->frame_size, 
				m_aCodecCtx->sample_rate,
				m_aCodecCtx->sample_fmt, 
				&m_aCodecCtx->ch_layout,
				m_aFrame);

			if (m_aFrame)
			{
				FillAudioFrame(m_aFrame, n_Data, n_nSize);
				m_aFrame->pts = m_naFrameIndex * m_aFrame->nb_samples;
				m_naFrameIndex++;
			}
		}


		if (Frame)
		{
			ret = ReceiveData(n_eMediaType, EDataType::DT_Frame, Frame);
			av_frame_unref(Frame);
		}

		return ret;
	}

	void CIOPcm::FillVideoFrame(AVFrame* n_Frame,
		const void* n_Data, const int n_nSize) const
	{
		int nPlanes = VideoPlanes();

		if (1 == nPlanes)
		{
			int nLineBytes = n_Frame->width * BytesPerPixel();
			if (nLineBytes < n_Frame->linesize[0])
			{
				for (int i = 0; i < n_Frame->height; i++)
				{
					memcpy_s(
						n_Frame->data[0] + i * n_Frame->linesize[0],
						n_Frame->linesize[0],
						(unsigned char*)n_Data + i * nLineBytes,
						nLineBytes);
				}
			}
			else
				memcpy_s(n_Frame->data[0], n_nSize, n_Data, n_nSize);
		}
	}

	void CIOPcm::FillAudioFrame(AVFrame* n_Frame, 
		const void* n_Data, const int n_nSize) const
	{
		int nIsPlanar = IsAudioPlanar();
		int nBytesPerSample = BytesPerSample();

		if (0 == nIsPlanar)
		{
			memcpy_s(n_Frame->data[0], n_Frame->linesize[0],
				n_Data, n_nSize);
		}
		else
		{
			uint8_t* ptr = (uint8_t*)n_Data;
			for (int i = 0; i < n_Frame->nb_samples; i++)
			{
				for (int j = 0; j < ChannelCount(); j++)
				{
					memcpy_s(n_Frame->data[j] + i * nBytesPerSample,
						nBytesPerSample,
						ptr,
						nBytesPerSample);
					ptr += nBytesPerSample;
				}
			}
		}
	}

	const int CIOPcm::VideoPlanes() const
	{
		if (!m_vCodecCtx) return 0;
		return av_pix_fmt_count_planes(m_vCodecCtx->pix_fmt);
	}

	const int CIOPcm::BytesPerPixel() const
	{
		if (!m_vCodecCtx) return 0;
		auto desc = av_pix_fmt_desc_get(m_vCodecCtx->pix_fmt);
		return av_get_bits_per_pixel(desc) / 8;
	}

	const int CIOPcm::IsAudioPlanar() const
	{
		if (!m_aCodecCtx) return -1;
		return av_sample_fmt_is_planar(m_aCodecCtx->sample_fmt);
	}

	const int CIOPcm::BytesPerSample() const
	{
		if (!m_aCodecCtx) return 0;
		return av_get_bytes_per_sample(m_aCodecCtx->sample_fmt);
	}

	const int CIOPcm::ChannelCount() const
	{
		if (!m_aCodecCtx) return 0;
		return m_aCodecCtx->ch_layout.nb_channels;
	}

}


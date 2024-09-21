#include "Filter/Filter.h"
#include "Util/Debug.h"


namespace avstudio
{
	IFilter::~IFilter()
	{
		Release();
	}

	void IFilter::Init(AVCodecContext* n_CodecContext)
	{
		m_CodecCtx = n_CodecContext;

		if (m_CodecCtx->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			m_Frame = FFrame::VideoFrame(
				m_CodecCtx->width,
				m_CodecCtx->height,
				m_CodecCtx->pix_fmt,
				m_Frame
			);
		}
		else if (m_CodecCtx->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			m_Frame = FFrame::AudioFrame(
				m_CodecCtx->frame_size,
				m_CodecCtx->sample_rate,
				m_CodecCtx->sample_fmt,
				&m_CodecCtx->ch_layout,
				m_Frame
			);
		}
	}

	void IFilter::Push(const unsigned int n_nIndex, AVFrame* n_Frame)
	{
		auto nIndex = FindInputIndex(n_nIndex);
		m_FilterGraph.Push(nIndex, n_Frame);
	}

	int IFilter::Pop(std::function<int(AVFrame*)> func)
	{
		int ret = -1;

		if (!m_Frame) return ret;

		auto nIndex = FindOutputIndex();

		while ((ret = m_FilterGraph.Pop(nIndex, m_Frame)) >= 0)
		{
			if (func) func(m_Frame);
			av_frame_unref(m_Frame);
		}

		if (ret == AVERROR_EOF && func)
			func(nullptr);

		return ret;
	}

	bool IFilter::IsValid() const
	{
		return m_FilterGraph.IsValid();
	}

	void IFilter::Dump() const
	{
		AVDebug("Graphic:\n %s\n",
			avfilter_graph_dump(m_FilterGraph.Graph, nullptr));
	}

	void IFilter::Release()
	{
		m_FilterGraph.Release();
		if (m_Frame) av_frame_free(&m_Frame);
	}

}


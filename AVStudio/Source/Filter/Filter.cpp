#include "Filter/Filter.h"
#include "Util/Debug.h"


namespace avstudio
{
	IFilter::~IFilter()
	{
		Release();
	}

	void IFilter::Init(std::shared_ptr<FCodecContext> n_CodecContext)
	{
		m_CodecCtx = n_CodecContext;

		auto Ctx = m_CodecCtx->Context;

		if (Ctx->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			m_Frame = FFrame::VideoFrame(
				Ctx->width,
				Ctx->height,
				n_CodecContext->GetPixelFormat(),
				m_Frame
			);
		}
		else if (Ctx->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			m_Frame = FFrame::AudioFrame(
				Ctx->frame_size,
				Ctx->sample_rate,
				Ctx->sample_fmt,
				&Ctx->ch_layout,
				m_Frame
			);
		}

		InitGraph();
	}

	void IFilter::Push(const unsigned int n_nIndex, AVFrame* n_Frame)
	{
		if (m_BufferCtx) Push(n_Frame);
		else
		{
			auto nIndex = FindInputIndex(n_nIndex);
			m_FilterGraph.Push(nIndex, n_Frame);
		}
	}

	void IFilter::Push(AVFrame* n_Frame)
	{
		if (m_BufferCtx) m_FilterGraph.Push(m_BufferCtx, n_Frame);
	}

	int IFilter::Pop(std::function<int(AVFrame*)> func)
	{
		int ret = -1;

		if (!m_Frame || !m_SinkCtx) return ret;

		while ((ret = m_FilterGraph.Pop(m_SinkCtx, m_Frame)) >= 0)
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

		m_BufferCtx = nullptr;
		m_SinkCtx = nullptr;
		m_FilterCtx = nullptr;
		m_CodecCtx = nullptr;
		
		if (m_Frame) av_frame_free(&m_Frame);
	}

	void IFilter::InitGraph()
	{
		ThrowExceptionExpr(!m_CodecCtx, "Call Init first\n");

		m_FilterGraph.AllocGraph();

		std::string sBuffer = "buffer";

		if (m_CodecCtx->Context->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
			sBuffer = "abuffer";

		m_BufferCtx = m_FilterGraph.BuildContext(sBuffer.c_str(), "src", m_CodecCtx);

		sBuffer += "sink";
		m_SinkCtx = m_FilterGraph.BuildContext(sBuffer.c_str(),
			"sink", nullptr);

		m_FilterCtx = m_BufferCtx;
	}

	unsigned int IFilter::FindInputIndex(const unsigned int n_nIndex)
	{
		return 0;
	}

}


#include "Filter/AudioMixFilter.h"
#include "Util/Debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	CAudioMixFilter::~CAudioMixFilter()
	{
	}

	void CAudioMixFilter::BuildInputFilter(unsigned int n_nInputSize)
	{
		ThrowExceptionExpr(!m_FilterGraph.Graph, "Call Init first\n");

		auto mixCtx = m_FilterGraph.BuildContext("amix", "mix");
		FilterMixOption(mixCtx, n_nInputSize);
		m_FilterGraph.Link(mixCtx, 0, m_SinkCtx, 0);

		char szBuffer[10] = { 0 };
		char szOption[512] = { 0 };
		char szLayout[64] = { 0 };

		for (unsigned int i = 0; i < n_nInputSize; i++)
		{
			memset(szBuffer, 0, sizeof(szBuffer));
			sprintf_s(szBuffer, sizeof(szBuffer), "buffer%d", i);

			// Input node
#if 1
			auto ctx = m_FilterGraph.BuildContext("abuffer", szBuffer, m_CodecCtx);
#else
			auto ctx = m_FilterGraph.AllocContext("abuffer", szBuffer);

			memset(szLayout, 0, sizeof(szLayout));
			av_channel_layout_describe(&m_CodecCtx->ch_layout, szLayout,
				sizeof(szLayout));

			memset(szOption, 0, sizeof(szOption));
			snprintf(szOption, sizeof(szOption),
				"sample_fmt=%s:sample_rate=%d:channel_layout=%s:channels=%d:time_base=%d/%d",
				av_get_sample_fmt_name(m_CodecCtx->sample_fmt), 
				m_CodecCtx->sample_rate,
				szLayout,
				m_CodecCtx->ch_layout.nb_channels,
				m_CodecCtx->time_base.num,
				m_CodecCtx->time_base.den);
			m_FilterGraph.InitContext(ctx, szOption);
#endif
			m_FilterGraph.Link(ctx, 0, mixCtx, i);
		}

		m_FilterGraph.GraphConfig();

		Dump();

		//for (unsigned int i = 0; i < m_FilterGraph.Graph->nb_filters; i++)
		//{
		//	auto filter = m_FilterGraph.Graph->filters[i];
		//	AVDebug("Filter [%d]: %s\n", i, filter->name);
		//}
	}

	void CAudioMixFilter::InitGraph()
	{
		ThrowExceptionExpr(!m_CodecCtx, "Call Init first\n");

		m_FilterGraph.AllocGraph();

		// Output node
		m_SinkCtx = m_FilterGraph.BuildContext("abuffersink", "sink", nullptr);
	}

	unsigned int CAudioMixFilter::FindInputIndex(const unsigned int n_nIndex)
	{
		// 0: sink filter context
		// 1: mix filter context
		// 2: buffer 0 filter context
		// 3: buffer 1 filter context
		// .....
		return n_nIndex + 2;
	}

	void CAudioMixFilter::FilterMixOption(AVFilterContext* n_FilterContext,
		const int n_nInput)
	{
		av_opt_set_int(n_FilterContext, "inputs", n_nInput, AV_OPT_SEARCH_CHILDREN);
		av_opt_set_int(n_FilterContext, "dropout_transition", 0,
			AV_OPT_SEARCH_CHILDREN);
		av_opt_set(n_FilterContext, "duration", "longest",
			AV_OPT_SEARCH_CHILDREN);
	}

}


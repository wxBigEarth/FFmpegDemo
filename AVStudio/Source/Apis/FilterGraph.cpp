#include "Apis/FilterGraph.h"
#include "Util/Debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	FFilterGraph::~FFilterGraph()
	{
		Release();
	}

	AVFilterGraph* FFilterGraph::AllocGraph()
	{
		if (!Graph) Graph = avfilter_graph_alloc();
		ThrowExceptionExpr(!Graph, "Fail to create filter graph.\n");

		return Graph;
	}

	AVFilterContext* FFilterGraph::AllocContext(const char* n_szFilterName,
		const char* n_szContextName) const
	{
		ThrowExceptionExpr(!Graph,
			"You should call function AllocGraph first.\n");

		const AVFilter* Filter = avfilter_get_by_name(n_szFilterName);
		ThrowExceptionExpr(!Filter, "Fail to find the filter: %s.\n", n_szFilterName);

		AVFilterContext* Context = avfilter_graph_alloc_filter(Graph, 
			Filter, n_szContextName);

		ThrowExceptionExpr(!Context, 
			"Fail to allocate the AVFilterContext instance: .\n", n_szContextName);

		return Context;
	}

	AVFilterContext* FFilterGraph::BuildContext(const char* n_szFilterName, 
		const char* n_szContextName, AVCodecContext* n_CodecContext)
	{
		AVFilterContext* FilterContext = AllocContext(n_szFilterName, n_szContextName);

		if (n_CodecContext)
		{
			if (n_CodecContext->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
				FilterVideoOption(FilterContext, n_CodecContext);
			else if (n_CodecContext->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
				FilterAudioOption(FilterContext, n_CodecContext);
		}

		InitContext(FilterContext);

		return FilterContext;
	}

	void FFilterGraph::InitContext(AVFilterContext* n_FilterContext,
		AVDictionary* n_Options)
	{
		int ret = avfilter_init_dict(n_FilterContext, &n_Options);
		if (n_Options) av_dict_free(&n_Options);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to initialize the filter.\n");
	}

	void FFilterGraph::InitContext(AVFilterContext* n_FilterContext, 
		const char* n_szArgs)
	{
		int ret = avfilter_init_str(n_FilterContext, n_szArgs);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to initialize the filter.\n");
	}

	void FFilterGraph::InitContext(AVFilterContext* n_FilterContext)
	{
		InitContext(n_FilterContext, (const char*)nullptr);
	}

	void FFilterGraph::Link(AVFilterContext* n_SrcFilterContext, unsigned n_nSrcpad, 
		AVFilterContext* n_DstFilterContext, unsigned n_nDstpad)
	{
		int ret = avfilter_link(n_SrcFilterContext, n_nSrcpad,
			n_DstFilterContext, n_nDstpad);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to connecting filters.\n");
	}

	void FFilterGraph::GraphConfig(void* arg_ctx /*= nullptr*/) const
	{
		int ret = avfilter_graph_config(Graph, arg_ctx);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to config the filter graph.\n");
	}

	bool FFilterGraph::IsValid() const
	{
		return Graph;
	}

	AVFilterContext* FFilterGraph::FindFilterContext(const char* n_szInstName) const
	{
		if (!Graph) return nullptr;
		return avfilter_graph_get_filter(Graph, n_szInstName);
	}

	AVFilterContext* FFilterGraph::FindFilterContext(
		const unsigned int n_nGraphIndex) const
	{
		AVFilterContext* ctx = nullptr;
		if (!Graph) return ctx;

		if (Graph->nb_filters > n_nGraphIndex)
		{
			ctx = Graph->filters[n_nGraphIndex];
		}

		return ctx;
	}

	AVFilterContext* FFilterGraph::FindFilterContext(const unsigned int n_nGraphIndex, 
		const unsigned int n_nInputIndex) const
	{
		AVFilterContext* ctx = nullptr;
		if (!Graph) return ctx;

		if (Graph->nb_filters > n_nGraphIndex)
		{
			auto Link = Graph->filters[n_nGraphIndex]->inputs;
			if (Graph->filters[n_nGraphIndex]->nb_inputs > n_nInputIndex)
				ctx = Link[n_nInputIndex]->src;
		}

		return ctx;
	}

	void FFilterGraph::Push(AVFilterContext* n_FilterContext, AVFrame* n_Frame)
	{
		int ret = av_buffersrc_add_frame(n_FilterContext, n_Frame);

		ThrowExceptionCodeExpr(ret < 0, ret, 
			"Fail to submit the frame to the filtergraph.\n");
	}

	//void FFilter::Push(const char* n_FilterName, AVFrame* n_Frame)
	//{
	//	if (!m_FilterGraph) return;

	//	for (unsigned int i = 0;i < m_FilterGraph->nb_filters; i++)
	//	{
	//		if (0 == strcmp(n_FilterName, m_FilterGraph->filters[i]->name))
	//		{
	//			Push(m_FilterGraph->filters[i], n_Frame);
	//			break;
	//		}
	//	}
	//}

	void FFilterGraph::Push(const unsigned int n_nIndex, AVFrame* n_Frame)
	{
		if (!Graph) return;
		if (n_nIndex < Graph->nb_filters)
			Push(Graph->filters[n_nIndex], n_Frame);
	}

	int FFilterGraph::Pop(const unsigned int n_nIndex, AVFrame* n_Frame) const
	{
		if (!Graph || n_nIndex >= Graph->nb_filters)
			return -1;

		int ret = av_buffersink_get_frame(
			Graph->filters[n_nIndex], n_Frame);

		return ret;
	}

	void FFilterGraph::Release()
	{
		if (Graph) avfilter_graph_free(&Graph);
	}

	void FilterVideoOption(AVFilterContext* n_FilterContext, 
		AVCodecContext* n_CodecContext)
	{
		av_opt_set_int(n_FilterContext, "width", n_CodecContext->width,
			AV_OPT_SEARCH_CHILDREN);
		av_opt_set_int(n_FilterContext, "height", n_CodecContext->height,
			AV_OPT_SEARCH_CHILDREN);
		av_opt_set_q(n_FilterContext, "time_base", n_CodecContext->time_base,
			AV_OPT_SEARCH_CHILDREN);
		av_opt_set(n_FilterContext, "pix_fmt", 
			av_get_pix_fmt_name(n_CodecContext->pix_fmt), AV_OPT_SEARCH_CHILDREN);
	}

	void FilterAudioOption(AVFilterContext* n_FilterContext, 
		AVCodecContext* n_CodecContext)
	{
		char szLayout[64] = { 0 };
		av_channel_layout_describe(&n_CodecContext->ch_layout, szLayout, 
			sizeof(szLayout));

		av_opt_set(n_FilterContext, "channel_layout", szLayout,
			AV_OPT_SEARCH_CHILDREN);
		av_opt_set(n_FilterContext, "sample_fmt", 
			av_get_sample_fmt_name(n_CodecContext->sample_fmt),
			AV_OPT_SEARCH_CHILDREN);
		av_opt_set_q(n_FilterContext, "time_base", n_CodecContext->time_base,
			AV_OPT_SEARCH_CHILDREN);
		av_opt_set_int(n_FilterContext, "sample_rate", n_CodecContext->sample_rate, 
			AV_OPT_SEARCH_CHILDREN);
	}

}

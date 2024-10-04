#ifndef __FILTERGRAPH_H__
#define __FILTERGRAPH_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	struct FFilterGraph
	{
		FFilterGraph() = default;
		~FFilterGraph();

		/* Create a new filter graph, which will contain all the filters. */
		AVFilterGraph* AllocGraph();

		/* Create the filter. */
		AVFilterContext* AllocContext(const char* n_szFilterName,
			const char* n_szContextName) const;

		/* Create the filter. */
		AVFilterContext* BuildContext(const char* n_szFilterName,
			const char* n_szContextName, AVCodecContext* n_CodecContext = nullptr);

		/* Now initialize the filter. */
		void InitContext(AVFilterContext* n_FilterContext, 
			AVDictionary* n_Options);

		/* Now initialize the filter. */
		/* n_szArgs: a string of the form key1=value1:key2=value2....*/
		void InitContext(AVFilterContext* n_FilterContext,
			const char* n_szArgs);

		/* Now initialize the filter. */
		void InitContext(AVFilterContext* n_FilterContext);

		/* Connect the filters; */
		void Link(AVFilterContext* n_SrcFilterContext, unsigned n_nSrcpad,
			AVFilterContext* n_DstFilterContext, unsigned n_nDstpad);

		/* Configure the graph. */
		void GraphConfig(void* arg_ctx = nullptr) const;

		bool IsValid() const;

		/* Find filter context */
		AVFilterContext* FindFilterContext(const char* n_szInstName) const;

		/* Find filter context */
		AVFilterContext* FindFilterContext(
			const unsigned int n_nGraphIndex) const;

		/* Find filter context */
		AVFilterContext* FindFilterContext(const unsigned int n_nGraphIndex, 
			const unsigned int n_nInputIndex) const;

		/* Send the frame to the input of the filter graph. */
		void Push(AVFilterContext* n_FilterContext, AVFrame* n_Frame);
		//void Push(const char* n_FilterName, AVFrame* n_Frame);

		/* Send the frame to the input of the filter graph. */
		void Push(const unsigned int n_nIndex, AVFrame* n_Frame);

		/* Get AVFrame from Filter. */
		int Pop(const unsigned int n_nIndex, AVFrame* n_Frame) const;

		void Release();

		AVFilterGraph*	Graph = nullptr;

	protected:
	};

	void FilterVideoOption(AVFilterContext* n_FilterContext,
		AVCodecContext* n_CodecContext);
	void FilterAudioOption(AVFilterContext* n_FilterContext,
		AVCodecContext* n_CodecContext);
}
#endif // __FILTERGRAPH_H__

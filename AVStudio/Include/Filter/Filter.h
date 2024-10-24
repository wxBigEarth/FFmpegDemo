#ifndef __FILTER_H__
#define __FILTER_H__
#include <functional>
#include <memory>
#include "Apis/CodecContext.h"
#include "Apis/FilterGraph.h"
#include "Apis/Frame.h"


namespace avstudio
{
	class IFilter
	{
	public:
		IFilter() = default;
		~IFilter();

		// std::shared_ptr<FCodecContext> n_CodecContext:
		//	Point to Codec Context of output context
		void Init(std::shared_ptr<FCodecContext> n_CodecContext);

		/* Send the frame to the input of the filter graph. */
		void Push(const unsigned int n_nIndex, AVFrame* n_Frame);

		/* Send the frame to the input of the filter graph. */
		void Push(AVFrame* n_Frame);

		/* Get AVFrame from Filter. */
		int Pop(std::function<int(AVFrame*)> func);

		bool IsValid() const;

		void Dump() const;

		void Release();

	protected:
		// Init filter graph
		virtual void InitGraph();

		// The index of input node in the graph
		virtual unsigned int FindInputIndex(const unsigned int n_nIndex);

	protected:
		FFilterGraph		m_FilterGraph;

		// Input node
		AVFilterContext*	m_BufferCtx = nullptr;

		// Output node
		AVFilterContext*	m_SinkCtx = nullptr;

		// Current node
		AVFilterContext*	m_FilterCtx = nullptr;

		// Point to Codec Context of output context
		std::shared_ptr<FCodecContext>	m_CodecCtx = nullptr;
		AVFrame*			m_Frame = nullptr;
	};

}

#endif //!__FILTER_H__

#ifndef __FILTER_H__
#define __FILTER_H__
#include <functional>
#include "Apis/FilterGraph.h"
#include "Apis/Frame.h"


namespace avstudio
{
	class IFilter
	{
	public:
		IFilter() = default;
		~IFilter();

		// AVCodecContext* n_CodecContext:
		//	Point to Codec Context of output context
		void Init(AVCodecContext* n_CodecContext);

		/* Send the frame to the input of the filter graph. */
		void Push(const unsigned int n_nIndex, AVFrame* n_Frame);

		/* Get AVFrame from Filter. */
		int Pop(std::function<int(AVFrame*)> func);

		bool IsValid() const;

		void Dump() const;

		void Release();

	protected:
		// The index of input node in the graph
		virtual unsigned int FindInputIndex(const unsigned int n_nIndex) = 0;
		// The index of output node in the graph
		virtual unsigned int FindOutputIndex() = 0;

	protected:
		FFilterGraph	m_FilterGraph;
		// Point to Codec Context of output context
		AVCodecContext* m_CodecCtx = nullptr;
		AVFrame*		m_Frame = nullptr;
	};

}

#endif //!__FILTER_H__

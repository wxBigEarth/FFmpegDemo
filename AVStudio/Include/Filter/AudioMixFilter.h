#ifndef _AUDIOMIXFILTER_H__
#define _AUDIOMIXFILTER_H__
#include "Filter/Filter.h"



namespace avstudio
{
	class CAudioMixFilter : public IFilter
	{
	public:
		CAudioMixFilter() = default;
		~CAudioMixFilter();

		void InitGraph(unsigned int n_nInputSize);

	protected:
		// The index of input node in the graph
		unsigned int FindInputIndex(const unsigned int n_nIndex) override;
		// The index of output node in the graph
		unsigned int FindOutputIndex() override;

		void FilterMixOption(AVFilterContext* n_FilterContext,
			const int n_nInput);

	protected:
	};

}


#endif	// !_AUDIOMIXFILTER_H__

#ifndef __AUDIOMIXFILTER_H__
#define __AUDIOMIXFILTER_H__
#include "Filter/Filter.h"



namespace avstudio
{
	class CAudioMixFilter : public IFilter
	{
	public:
		CAudioMixFilter() = default;
		~CAudioMixFilter();

		void BuildInputFilter(unsigned int n_nInputSize);

	protected:
		void InitGraph() override;

		// The index of input node in the graph
		unsigned int FindInputIndex(const unsigned int n_nIndex) override;

		void FilterMixOption(AVFilterContext* n_FilterContext,
			const int n_nInput);

	protected:
	};

}


#endif	// !__AUDIOMIXFILTER_H__

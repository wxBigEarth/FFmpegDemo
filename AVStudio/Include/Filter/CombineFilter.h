#ifndef __COMBINEFILTER_H__
#define __COMBINEFILTER_H__
#include "Filter/Filter.h"



namespace avstudio
{
	class CCombineFilter : public IFilter
	{
	public:
		CCombineFilter() = default;
		~CCombineFilter();

		// Add filter node
		// n_bLast: true if append the last filter
		void AppendFilter(const char* n_szName, const bool n_bLast = false);

		// Add filter node
		// n_bLast: true if append the last filter
		// const char* n_szArgs: format like like
		//	"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d"
		void AppendFilter(const char* n_szName, const char* n_szArgs, 
			const bool n_bLast = false);

		// Add filter node
		// n_bLast: true if append the last filter
		void AppendFilter(const char* n_szName, AVDictionary* n_Options,
			const bool n_bLast = false);
		
	protected:
		void LastFilter();
	};

}


#endif	// !__COMBINEFILTER_H__

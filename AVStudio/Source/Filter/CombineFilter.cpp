#include "Filter/CombineFilter.h"
#include "Util/Debug.h"


namespace avstudio
{
	CCombineFilter::~CCombineFilter()
	{

	}

	void CCombineFilter::AppendFilter(const char* n_szName, const bool n_bLast)
	{
		AppendFilter(n_szName, (const char*)nullptr, n_bLast);
	}

	void CCombineFilter::AppendFilter(const char* n_szName, 
		const char* n_szArgs, const bool n_bLast)
	{
		if (m_FilterCtx == m_SinkCtx) return;

		auto Ctx = m_FilterGraph.AllocContext(n_szName, n_szName);
		m_FilterGraph.InitContext(Ctx, n_szArgs);
		m_FilterGraph.Link(m_FilterCtx, 0, Ctx, 0);
		m_FilterCtx = Ctx;

		if (n_bLast) LastFilter();
	}

	void CCombineFilter::AppendFilter(const char* n_szName, 
		AVDictionary* n_Options, const bool n_bLast)
	{
		if (m_FilterCtx == m_SinkCtx) return;

		auto Ctx = m_FilterGraph.AllocContext(n_szName, n_szName);
		m_FilterGraph.InitContext(Ctx, n_Options);
		m_FilterGraph.Link(m_FilterCtx, 0, Ctx, 0);
		m_FilterCtx = Ctx;

		if (n_bLast) LastFilter();
	}

	void CCombineFilter::LastFilter()
	{
		m_FilterGraph.Link(m_FilterCtx, 0, m_SinkCtx, 0);
		m_FilterCtx = m_SinkCtx;
		m_FilterGraph.GraphConfig();

		Dump();
	}

}

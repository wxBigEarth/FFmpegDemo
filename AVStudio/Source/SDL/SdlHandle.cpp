#include "SDL/SdlHandle.h"


namespace avstudio
{
	void ISdlHandle::SDL_Update(double n_dCur, double n_dMax) const
	{
		if (m_fnUpdate) m_fnUpdate(n_dCur, n_dMax);
	}

	void ISdlHandle::SetUpdateCallback(std::function<void(double, double)> func)
	{
		m_fnUpdate = func;
	}
}

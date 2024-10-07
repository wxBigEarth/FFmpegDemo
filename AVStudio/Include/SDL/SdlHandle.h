#ifndef __SDLHANDLE_H__
#define __SDLHANDLE_H__
#include <functional>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/frame.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	class ISdlHandle
	{
	public:
		// Read frame data
		virtual AVFrame* SDL_ReadFrame(AVMediaType n_eMediaType) = 0;
		// What to do with the frame after playing, EG: free n_Frame memory
		virtual void SDL_ReadEnd(AVFrame* n_Frame) {}
		// Quit
		virtual void SDL_Stop() {}
		/*
		* Update video
		*	double n_dCur: current played time
		*	double n_dMax: max duration, if 0, will not draw progress bar.
		*/
		void SDL_Update(double n_dCur, double n_dMax) const;
		// Set callback function for updating
		void SetUpdateCallback(std::function<void(double, double)> func);

	protected:
		std::function<void(double, double)> m_fnUpdate = nullptr;
	};

}

#endif //!__SDLHANDLE_H__
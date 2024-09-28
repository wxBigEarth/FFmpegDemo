#ifndef __SDL_H__
#define __SDL_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <SDL.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	struct FSdl
	{
		FSdl() = default;
		~FSdl();

		void Init(const unsigned char n_nMediaMask);

		// Create video render window
		// Just support YUV pixel format
		void InitVideo(const char* n_szTitle, 
			const int n_nWidth, const int n_nHeight);

		// Create video render window
		// n_WinId: MFC window HWND or QT winID
		void InitVideo(const void* n_WinId);

		// Init audio parameter
		void InitAudio(AVCodecContext* n_CodecContext,
			SDL_AudioCallback n_Cb, void* n_Param);

		// Input YUV frame to render on the window
		void UpdateYUV(AVFrame* n_Frame);
		// Input audio frame to play audio
		// It's called by SDL_AudioCallback in InitAudio() function
		void UpdateAudio(AVFrame* n_Frame, 
			unsigned char* n_Stream, int n_nLen) const;

		const unsigned int Event();

		// Play
		void Play();
		// Pause
		void Pause();
		// Is pause now
		bool IsPause() const;

		void Release();

		// Reset the region that display video image
		void SetViewRect(int n_nLeft, int n_nTop, int n_nWidth, int n_nHeight);

	protected:
		void CreateTexture(unsigned int n_nPixFmt);

		void CreateRenderer(SDL_Window* n_Window);

	protected:
		SDL_Window*		m_Window = nullptr;
		SDL_Renderer*	m_Renderer = nullptr;
		SDL_Texture*	m_Texture = nullptr;

		SDL_Rect		m_Rect = { 0 };

		// If the input context is planar
		int				m_nPlanar = 0;
		// 0: play; 1: pause
		int				m_nPause = 0;
	};
}

#endif //!__SDL_H__

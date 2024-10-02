#ifndef __SDL_H__
#define __SDL_H__
#include <functional>
#include <memory>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
#include <SDL.h>
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
		// What to do with the frame after playing
		virtual void SDL_ReadEnd(AVFrame* n_Frame) {}
		// Quit
		virtual void SDL_Stop() {}
		// Update video 
		void SDL_Update() const { if (m_fnUpdate) m_fnUpdate(); }

		void SetUpdateCallback(std::function<void()> func) { m_fnUpdate = func; }

	protected:
		std::function<void()> m_fnUpdate = nullptr;
	};

	struct FSdl
	{
		FSdl() = default;
		~FSdl();

		void Init(const unsigned char n_nMediaMask, 
			std::shared_ptr<ISdlHandle> n_Handle);

		// Create video render window
		// Just support YUV pixel format
		void InitVideo(const char* n_szTitle, 
			const int n_nWidth, const int n_nHeight);

		// Create video render window
		// n_WinId: MFC window HWND or QT winID
		void InitVideo(const void* n_WinId);

		// Init audio parameter
		/*
		* Init audio parameter
		* The supported AVSampleFormat: 
		* 	AVSampleFormat::AV_SAMPLE_FMT_U8
		*	AVSampleFormat::AV_SAMPLE_FMT_U8P
		*	AVSampleFormat::AV_SAMPLE_FMT_S16
		*	AVSampleFormat::AV_SAMPLE_FMT_S16P
		*	AVSampleFormat::AV_SAMPLE_FMT_S32
		*	AVSampleFormat::AV_SAMPLE_FMT_S32P
		*	AVSampleFormat::AV_SAMPLE_FMT_FLT
		*	AVSampleFormat::AV_SAMPLE_FMT_FLTP
		*/
		void InitAudio(
			int n_nSampleRate,
			int n_nFrameSize,
			int n_nNbChannel,
			AVSampleFormat n_nSampleFormat);

		// Input YUV frame to render on the window
		void UpdateVideo(AVFrame* n_Frame);
		// Input audio frame to play audio
		// It's called by SDL_AudioCallback in InitAudio() function
		void UpdateAudio(AVFrame* n_Frame,
			unsigned char* n_Stream, int n_nLen) const;

		// SDL event, should call in the same thread as InitVieo
		const unsigned int Event();
		// Send event to display video
		void SendDisplayEvent() const;

		// Video callback PROC
		void VideoProc();

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

		// Audio callback
		static void AudioCallback(void* n_UserData,
			unsigned char* n_szStream, int n_nLen);

		// Audio callback PROC
		void AudioProc(unsigned char* n_szStream, int n_nLen);

	protected:
		SDL_Window*		m_Window = nullptr;
		SDL_Renderer*	m_Renderer = nullptr;
		SDL_Texture*	m_Texture = nullptr;
		SDL_Rect		m_Rect = { 0 };

		unsigned char	m_nMediaMask = 0;
		// Event that display video
		unsigned int	m_nDisplayEvent = 0;
		
		// Channel count
		int				m_nChannels = 1;
		// If the input context is planar
		int				m_nPlanar = 0;
		// The size of one sample
		int				m_nBytesPerSample = 1;
		// 0: play; 1: pause
		int				m_nPause = 0;

		std::shared_ptr<ISdlHandle>	m_SdlHandle = nullptr;
	};
}

#endif //!__SDL_H__

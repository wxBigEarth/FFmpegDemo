#ifndef __SDL_H__
#define __SDL_H__
#include <memory>
#include "SDL/SdlHandle.h"
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
	enum class ESdlStatus
	{
		SS_Play = 0,
		SS_Pause,
		SS_Stop,
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
			const int n_nWidth, const int n_nHeight,
			unsigned int n_nPixFmt = SDL_PIXELFORMAT_IYUV);

		// Create video render window
		// n_WinId: MFC window HWND or QT winID
		void InitVideo(const void* n_WinId, 
			unsigned int n_nPixFmt = SDL_PIXELFORMAT_IYUV);

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

		// SDL event, should call in the same thread as InitVieo
		const unsigned int Event();
		// Send event to display video
		void SendDisplayEvent() const;

		/*
		* Update
		*	double n_dCur: current played time
		*	double n_dMax: max duration
		*/
		void Update(double n_dCur, double n_dMax);

		// Play
		void Play();
		// Pause
		void Pause();
		// Is pause now
		bool IsPause() const;
		// Stop
		void Stop();

		void Release();

		// Reset the region that display video image
		void SetViewRect(int n_nLeft, int n_nTop, int n_nWidth, int n_nHeight);

	protected:
		void CreateTexture(unsigned int n_nPixFmt);
		void CreateRenderer(SDL_Window* n_Window);

		// Video callback PROC
		void VideoProc();

		// Audio callback
		static void AudioCallback(void* n_UserData,
			unsigned char* n_szStream, int n_nLen);

		// Audio callback PROC
		void AudioProc(unsigned char* n_szStream, int n_nLen);

		// Playing Process 
		void ProgressBarProc(double n_dCur, double n_dMax);

		// Input frame to render on the window, Default pixel format is YUV
		// it also support NV12, NV21 pixel format
		void UpdateVideo(AVFrame* n_Frame);
		// Input audio frame to play audio
		// It's called by SDL_AudioCallback in InitAudio() function
		void UpdateAudio(AVFrame* n_Frame,
			unsigned char* n_Stream, int n_nLen) const;

	protected:
		SDL_Window*		m_Window = nullptr;
		SDL_Renderer*	m_Renderer = nullptr;
		SDL_Texture*	m_Texture = nullptr;
		SDL_Rect		m_Rect = { 0 };

		// Indicate selected streams
		unsigned char	m_nMediaMask = 0;
		// Event that display video
		unsigned int	m_nDisplayEvent = 0;
		// Pixel format of SDL
		unsigned int	m_nPixFmt = SDL_PIXELFORMAT_IYUV;
		
		// Channel count
		int				m_nChannels = 1;
		// If the input context is planar
		int				m_nPlanar = 0;
		// The size of one sample
		int				m_nBytesPerSample = 1;
		// Status
		ESdlStatus		m_eStatus = ESdlStatus::SS_Stop;

		std::shared_ptr<ISdlHandle>	m_SdlHandle = nullptr;
	};
}

#endif //!__SDL_H__

#ifndef __SDL_H__
#define __SDL_H__
#include <memory>
#include "Util/Callback.h"
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

	struct FSdlData
	{
		AVFrame* Frame = nullptr;
		double dProgress = 0;
	};

	struct FSdl
	{
		FSdl() = default;
		~FSdl();

		void Init(const unsigned char n_nMediaMask);

		/*
		* Create video render window
		* If n_WinId is not null, use the exists window
		* Parameter
		*	n_WinId: MFC window HWND or QT winID
		*	n_nPixFmt: the pixel format of input video data
		*/
		void InitVideo(const char* n_szTitle, 
			const int n_nWidth, const int n_nHeight,
			const void* n_WinId = nullptr,
			unsigned int n_nPixFmt = SDL_PIXELFORMAT_IYUV);

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
			AVSampleFormat n_eSampleFormat);

		void CloseAudio();

		// Setup Callback function to fetch AVFrame
		void SetupCallback(FCallback<FSdlData, AVMediaType> n_cb);

		/*
		* SDL event, should be called in the same thread as InitVieo
		* return value:
		*	-1: Quit
		*/
		const int Event();

		// Send event to display video
		void SendDisplayEvent();

		// Update image with data
		void Update(const int n_nWidth, const int n_nHeight,
			const void* n_Data, const int n_nPitch);

		// Play
		void Play();
		// Pause
		void Pause();
		// Is pause now
		const bool IsPause() const;
		// Stop
		// Audio should be close in the thread same as which open audio
		void Stop(const bool n_bCloseAudio = true);
		// Is stopped now
		const bool IsStopped() const;

		void Release();

		// Reset the region that display video image
		void SetViewRect(int n_nLeft, int n_nTop, int n_nWidth, int n_nHeight);

		// Flush renderer and texture.
		void FlushRendererAndTexture();

	protected:
		void ReleaseWindow();

		void CreateTexture(unsigned int n_nPixFmt);
		void ReleaseTexture();
		void CreateRenderer(SDL_Window* n_Window);
		void ReleaseRenderer();

		void ReleaseMutex();

		// Video callback PROC
		void VideoProc();

		// Audio callback
		static void AudioCallback(void* n_UserData,
			unsigned char* n_szStream, int n_nLen);

		// Audio callback PROC
		void AudioProc(unsigned char* n_szStream, int n_nLen);

		// Playing Process 
		void ProgressBarProc(double n_dProgress);

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
		SDL_mutex*		m_Mutex = nullptr;
		SDL_DisplayMode	m_Dm = { 0 };

		bool			m_bIsAudioUsed = false;

		// Indicate selected streams
		unsigned char	m_nMediaMask = 0;
		// Event id that display video
		unsigned int	m_nDisplayEvent = 0;
		// Pixel format of SDL
		unsigned int	m_nPixFmt = SDL_PIXELFORMAT_IYUV;
		
		// If the input context is planar
		int				m_nPlanar = 0;
		// The size of one sample
		int				m_nBytesPerSample = 1;
		// Status
		ESdlStatus		m_eStatus = ESdlStatus::SS_Stop;

		FCallback<FSdlData, AVMediaType>	m_cb;
	};
}

#endif //!__SDL_H__

#ifndef __SDLPLAYER_H__
#define __SDLPLAYER_H__
#include "Sdl/Sdl.h"
#include "Core/WorkShop.h"
#include "IO/IOPlayer.h"
#include "Util/Thread.h"


namespace avstudio
{
	class CSdlPlayer : public Thread
	{
	public:
		CSdlPlayer();
		~CSdlPlayer();

		void Init(std::shared_ptr<FWorkShop> n_Output,
			const char* n_szTitle,
			const void* n_WinId = nullptr,
			unsigned int n_nPixFmt = SDL_PIXELFORMAT_IYUV);

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

		std::shared_ptr<FSdl> GetSdl();
		std::shared_ptr<CIOPlayer> GetIoHandle();

		// Set the max length of video/audio file
		void SetMaxLength(const double n_dLength);

		void Play();
		void Pause();
		const bool IsPause() const;

		void Release();

	protected:
		void Run() override;

		// Callback for CIOPlayer
		static void IoEvent(void* n_Param, EIOPEventId n_eId);
		void IoEventProc(EIOPEventId n_eId);

		// Callback for SDL
		static FSdlData SdlEvent(void* n_Param, AVMediaType n_eMediaType);
		FSdlData SdlEventProc(AVMediaType n_eMediaType);

		struct FAVInfo
		{
			int nWidth = 0;
			int nHeight = 0;
			const char* szTitle = nullptr;
			const void* WinId = nullptr;
			unsigned int nPixFmt = SDL_PIXELFORMAT_IYUV;

			int nSampleRate = 0;
			int nFrameSize = 0;
			int nNbChannel = 0;
			AVSampleFormat eSampleFormat = AVSampleFormat::AV_SAMPLE_FMT_NONE;
		};

	protected:
		double						m_dMaxLength = 0;

		FAVInfo						m_AVInfo = { 0 };

		std::shared_ptr<FSdl>		m_Sdl = nullptr;
		std::shared_ptr<CIOPlayer>	m_IoPlayer = nullptr;
	};
}

#endif // !__SDLPLAYER_H__

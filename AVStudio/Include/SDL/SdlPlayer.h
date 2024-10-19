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

		std::shared_ptr<FSdl> GetSdl();
		std::shared_ptr<CIOPlayer> GetIoHandle();

		// Set the max length of video/audio file
		void SetMaxLength(const double n_dLength);

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

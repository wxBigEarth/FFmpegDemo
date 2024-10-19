#include "Sdl/SdlPlayer.h"


namespace avstudio
{
	CSdlPlayer::CSdlPlayer()
	{
		// Init SDL
		FCallback<FSdlData, AVMediaType> sdlCb;
		sdlCb.Init(CSdlPlayer::SdlEvent, this);

		m_Sdl = std::make_shared<FSdl>();
		m_Sdl->SetupCallback(sdlCb);

		// Init IO
		FCallback<void, EIOPEventId> ioCb;
		ioCb.Init(CSdlPlayer::IoEvent, this);

		m_IoPlayer = std::make_shared<CIOPlayer>();
		m_IoPlayer->SetupCallback(ioCb);
	}

	CSdlPlayer::~CSdlPlayer()
	{
	}

	void CSdlPlayer::Init(std::shared_ptr<FWorkShop> n_Output,
		const char* n_szTitle,
		const void* n_WinId /*= nullptr*/,
		unsigned int n_nPixFmt /*= SDL_PIXELFORMAT_IYUV*/)
	{
		if (!n_Output) return;

		m_Sdl->Init(n_Output->GetMediaMask());

		m_AVInfo.szTitle = n_szTitle;
		m_AVInfo.WinId = n_WinId;
		m_AVInfo.nPixFmt = n_nPixFmt;

		if (n_Output->AudioParts.Codec && n_Output->AudioParts.Codec->Context)
		{
			auto Ctx = n_Output->AudioParts.Codec->Context;
			m_AVInfo.nSampleRate = Ctx->sample_rate;
			m_AVInfo.nFrameSize = Ctx->frame_size;
			m_AVInfo.nNbChannel = Ctx->ch_layout.nb_channels;
			m_AVInfo.eSampleFormat = Ctx->sample_fmt;
		}

		if (n_Output->VideoParts.Codec && n_Output->VideoParts.Codec->Context)
		{
			auto Ctx = n_Output->VideoParts.Codec->Context;
			m_AVInfo.nWidth = Ctx->width;
			m_AVInfo.nHeight = Ctx->height;
		}
	}

	std::shared_ptr<FSdl> CSdlPlayer::GetSdl()
	{
		return m_Sdl;
	}

	std::shared_ptr<CIOPlayer> CSdlPlayer::GetIoHandle()
	{
		return m_IoPlayer;
	}

	void CSdlPlayer::SetMaxLength(const double n_dLength)
	{
		m_dMaxLength = n_dLength;
	}

	void CSdlPlayer::Run()
	{
		if (m_AVInfo.eSampleFormat != AVSampleFormat::AV_SAMPLE_FMT_NONE)
		{
			m_Sdl->InitAudio(m_AVInfo.nSampleRate,
				m_AVInfo.nFrameSize,
				m_AVInfo.nNbChannel,
				m_AVInfo.eSampleFormat);
		}

		if (m_AVInfo.nWidth > 0 && m_AVInfo.nHeight > 0)
		{
			m_Sdl->InitVideo(m_AVInfo.szTitle, 
				m_AVInfo.nWidth,
				m_AVInfo.nHeight, 
				m_AVInfo.WinId, 
				m_AVInfo.nPixFmt);
		}

		m_Sdl->Play();

		while (!m_Sdl->IsStopped())
		{
			if (-1 == m_Sdl->Event())
			{
				m_IoPlayer->ForceStop();
				m_IoPlayer->Release2();
				break;
			}
			SDL_Delay(3);
		}

		m_Sdl->CloseAudio();
	}

	void CSdlPlayer::IoEvent(void* n_Param, EIOPEventId n_eId)
	{
		if (!n_Param) return;

		auto Player = (CSdlPlayer*)n_Param;
		Player->IoEventProc(n_eId);
	}

	void CSdlPlayer::IoEventProc(EIOPEventId n_eId)
	{
		switch (n_eId)
		{
		case EIOPEventId::PEI_UpdateVideo:
			m_Sdl->SendDisplayEvent();
			break;
		case EIOPEventId::PEI_EOF:
			m_Sdl->Stop(false);
			break;
		default:
			break;
		}
	}

	FSdlData CSdlPlayer::SdlEvent(void* n_Param, AVMediaType n_eMediaType)
	{
		if (!n_Param) return FSdlData{ nullptr, 0 };

		auto Player = (CSdlPlayer*)n_Param;
		return Player->SdlEventProc(n_eMediaType);
	}

	FSdlData CSdlPlayer::SdlEventProc(AVMediaType n_eMediaType)
	{
		FSdlData Data{};

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			m_IoPlayer->PopVideo(Data.Frame);
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			m_IoPlayer->PopAudio(Data.Frame);

		if (m_dMaxLength > 0)
			Data.dProgress = m_IoPlayer->PlayedTime() / m_dMaxLength;
		else
			Data.dProgress = -1;

		return Data;
	}

}


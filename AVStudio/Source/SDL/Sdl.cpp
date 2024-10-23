#include "SDL/Sdl.h"
#include "Util/Debug.h"
#include "Util/MediaMask.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/pixfmt.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	FSdl::~FSdl()
	{
		Release();
	}

	void FSdl::Init(const unsigned char n_nMediaMask)
	{
		uint32_t nFlags = 0;

		if (IsCompriseMedia(n_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
			nFlags |= SDL_INIT_VIDEO;
		else if (IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
			SDL_QuitSubSystem(SDL_INIT_VIDEO);

		if (IsCompriseMedia(n_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
			nFlags |= SDL_INIT_AUDIO;
		else if (IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
			SDL_QuitSubSystem(SDL_INIT_AUDIO);

		if (nFlags != 0)
		{
			int ret = SDL_Init(nFlags);
			ThrowExceptionExpr(ret,
				"Fail to initialize SDL: %s.\n", SDL_GetError());
		}

		m_nMediaMask = n_nMediaMask;
	}

	void FSdl::InitVideo(const char* n_szTitle,
		const int n_nWidth, const int n_nHeight, 
		const void* n_WinId /*= nullptr*/,
		unsigned int n_nPixFmt /*= SDL_PIXELFORMAT_IYUV*/)
	{
		if (m_Window) return;

		bool b = IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (!b) return;

		if (n_WinId)
			m_Window = SDL_CreateWindowFrom(n_WinId);
		else
			m_Window = SDL_CreateWindow(n_szTitle,
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED,
				n_nWidth,
				n_nHeight,
				SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

		ThrowExceptionExpr(!m_Window, 
			"Fail to create window: %s.\n", SDL_GetError());

		memset(&m_Rect, 0, sizeof(SDL_Rect));
		SDL_GetWindowSize(m_Window, &m_Rect.w, &m_Rect.h);
		SetRotateCenter();

		CreateRenderer(m_Window);
		CreateTexture(n_nPixFmt);
		
		if (m_nDisplayEvent == 0) m_nDisplayEvent = SDL_RegisterEvents(1);
		if (!m_Mutex) m_Mutex = SDL_CreateMutex();
	}

	void FSdl::InitAudio(
		int n_nSampleRate,
		int n_nFrameSize,
		int n_nNbChannel,
		AVSampleFormat n_eSampleFormat)
	{
		if (m_bIsAudioUsed) return;

		bool b = IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO);
		if (!b) return;

		SDL_AudioSpec AudioSpec = { 0 };

		AudioSpec.freq = n_nSampleRate;
		AudioSpec.channels = n_nNbChannel;
		AudioSpec.silence = 0;
		AudioSpec.samples = n_nFrameSize;
		AudioSpec.callback = AudioCallback;
		AudioSpec.userdata = this;

		switch (n_eSampleFormat)
		{
		case AVSampleFormat::AV_SAMPLE_FMT_U8:
		case AVSampleFormat::AV_SAMPLE_FMT_U8P:
			AudioSpec.format = AUDIO_U8;
			break;
		case AVSampleFormat::AV_SAMPLE_FMT_S16:
		case AVSampleFormat::AV_SAMPLE_FMT_S16P:
			AudioSpec.format = AUDIO_S16;
			break;
		case AVSampleFormat::AV_SAMPLE_FMT_S32:
		case AVSampleFormat::AV_SAMPLE_FMT_S32P:
			AudioSpec.format = AUDIO_S32;
			break;
		case AVSampleFormat::AV_SAMPLE_FMT_FLT:
		case AVSampleFormat::AV_SAMPLE_FMT_FLTP:
			AudioSpec.format = AUDIO_F32;
			break;
		default:
			ThrowException("SDL do not support %s\n", 
				av_get_sample_fmt_name(n_eSampleFormat));
			break;
		}

		int ret = SDL_OpenAudio(&AudioSpec, nullptr);
		ThrowExceptionExpr(ret, "Fail to open audio: %s.\n", SDL_GetError());

		m_nPlanar = av_sample_fmt_is_planar(n_eSampleFormat);
		m_nBytesPerSample = av_get_bytes_per_sample(n_eSampleFormat);;

		m_bIsAudioUsed = true;
	}

	void FSdl::CloseAudio()
	{
		if (m_bIsAudioUsed)
		{
			SDL_PauseAudio(1);
			SDL_CloseAudio();
		}

		m_bIsAudioUsed = false;
	}

	void FSdl::SetupCallback(FCallback<FSdlData, AVMediaType> n_cb)
	{
		m_cb = n_cb;
	}

	const int FSdl::Event()
	{
		if (!m_Window) return 0;

		SDL_Event Event;

		while (SDL_PollEvent(&Event))
		{
			switch (Event.type) {
			case SDL_QUIT:
				//Stop();
				return -1;
				break;
			case SDL_KEYUP:
				if (Event.key.keysym.sym == SDLK_SPACE)
				{
					if (IsPause())
						Play();
					else
						Pause();
				}
				break;
			case SDL_WINDOWEVENT:
				if (Event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					FlushRendererAndTexture();
				break;
			default:
				if (m_nDisplayEvent == Event.type && m_Window)
					VideoProc();
				break;
			}

			//SDL_Delay(10);
		}

		return Event.type;
	}

	void FSdl::SendDisplayEvent()
	{
#if 0
		SDL_Event Event{};
		Event.type = m_nDisplayEvent;
		SDL_PushEvent(&Event);
#else
		VideoProc();
#endif
	}

	void FSdl::Update(const int n_nWidth, const int n_nHeight, 
		const void* n_Data, const int n_nPitch)
	{
		if (!n_Data || !m_Texture || !m_Renderer ||
			m_eStatus != ESdlStatus::SS_Play) return;

		SDL_LockMutex(m_Mutex);

		SDL_Rect rcFrame = { 0, 0, n_nWidth, n_nHeight };

		auto ret = SDL_UpdateTexture(m_Texture, &rcFrame, n_Data, n_nPitch);

		if (ret == 0)
		{
			SDL_RenderClear(m_Renderer);
			RendererCopy(rcFrame);
			SDL_RenderPresent(m_Renderer);
		}
		else
			AVDebug("Warning: Fail to render frame on the window,Error: %s\n",
				SDL_GetError());

		SDL_UnlockMutex(m_Mutex);
	}

	void FSdl::Play()
	{
		m_eStatus = ESdlStatus::SS_Play;
		if (m_bIsAudioUsed) SDL_PauseAudio(0);
	}

	void FSdl::Pause()
	{
		m_eStatus = ESdlStatus::SS_Pause;
		if (m_bIsAudioUsed) SDL_PauseAudio(1);
	}

	const bool FSdl::IsPause() const
	{
		return m_eStatus == ESdlStatus::SS_Pause;
	}

	void FSdl::Stop(const bool n_bCloseAudio)
	{
		if (m_eStatus == ESdlStatus::SS_Stop) return;
		if (n_bCloseAudio) CloseAudio();
		m_eStatus = ESdlStatus::SS_Stop;
	}

	const bool FSdl::IsStopped() const
	{
		return m_eStatus == ESdlStatus::SS_Stop;
	}

	void FSdl::Release()
	{
		Stop();
		CloseAudio();

		ReleaseTexture();
		ReleaseRenderer();
		ReleaseWindow();
		ReleaseMutex();
		SDL_Quit();

		memset(&m_Rect, 0, sizeof(SDL_Rect));
	}

	void FSdl::FlushRendererAndTexture()
	{
		if (!m_Window || !m_Texture || !m_Renderer)
			return;

		SDL_GetWindowSize(m_Window, &m_Rect.w, &m_Rect.h);
		SetRotateCenter();

		SDL_LockMutex(m_Mutex);
		ReleaseTexture();
		ReleaseRenderer();
		CreateRenderer(m_Window);
		CreateTexture(m_nPixFmt);
		SDL_UnlockMutex(m_Mutex);
	}

	void FSdl::SetViewRect(int n_nLeft, int n_nTop, int n_nWidth, int n_nHeight)
	{
		if (m_Window) return;

		int w = 0;
		int h = 0;

		SDL_GetWindowSize(m_Window, &w, &h);

		m_Rect.x = n_nLeft;
		m_Rect.y = n_nTop;

		if (m_Rect.x + n_nWidth > w)
			m_Rect.w = m_Rect.x + n_nWidth - w;
		else
			m_Rect.w = n_nWidth;

		if (m_Rect.y + n_nHeight > h)
			m_Rect.h = m_Rect.y + n_nHeight - h;
		else
			m_Rect.h = n_nHeight;

		SetRotateCenter();
	}

	void FSdl::SetRotation(double n_dDegree)
	{
		m_dRotation = n_dDegree;
	}

	void FSdl::SetRotateCenter(int n_nX, int n_nY)
	{
		m_RotateCenter.x = n_nX;
		if (m_RotateCenter.x < 0)
			m_RotateCenter.x = m_Rect.x + m_Rect.w / 2;

		m_RotateCenter.y = n_nY;
		if (m_RotateCenter.y < 0)
			m_RotateCenter.y = m_Rect.y + m_Rect.h / 2;
	}

	void FSdl::SetFlipType(SDL_RendererFlip n_eFlip)
	{
		m_RendererFlip = n_eFlip;
	}

	void FSdl::ReleaseWindow()
	{
		if (!m_Window) return;
		SDL_DestroyWindow(m_Window);
		m_Window = nullptr;
	}

	void FSdl::CreateTexture(unsigned int n_nPixFmt)
	{
		if (!m_Window) return;

		ReleaseTexture();

		int nWidth = 1920;
		int nHeight = 1080;

		if (m_Dm.refresh_rate == 0)
		{
			auto ret = SDL_GetDesktopDisplayMode(0, &m_Dm);
			if (ret == 0)
			{
				nWidth = m_Dm.w;
				nHeight = m_Dm.h;
			}
		}
		
		m_Texture = SDL_CreateTexture(m_Renderer,
			n_nPixFmt,
			SDL_TEXTUREACCESS_STREAMING,
			nWidth,
			nHeight);
		ThrowExceptionExpr(!m_Texture,
			"Fail to create texture: %s.\n", SDL_GetError());

		m_nPixFmt = n_nPixFmt;
	}

	void FSdl::ReleaseTexture()
	{
		if (!m_Texture) return;
		SDL_DestroyTexture(m_Texture);
		m_Texture = nullptr;
	}

	void FSdl::CreateRenderer(SDL_Window* n_Window)
	{
		m_Renderer = SDL_GetRenderer(m_Window);
		if (!m_Renderer) m_Renderer = SDL_CreateRenderer(n_Window, -1, 0);

		ThrowExceptionExpr(!m_Renderer,
			"Fail to create renderer: %s.\n", SDL_GetError());
	}

	void FSdl::ReleaseRenderer()
	{
		if (!m_Renderer) return;
		SDL_DestroyRenderer(m_Renderer);
		m_Renderer = nullptr;
	}

	void FSdl::ReleaseMutex()
	{
		if (!m_Mutex) return;
		SDL_DestroyMutex(m_Mutex);
		m_Mutex = nullptr;
	}

	void FSdl::RendererCopy(const SDL_Rect& n_rcTexture)
	{
		if (m_dRotation != 0 || m_RendererFlip != SDL_RendererFlip::SDL_FLIP_NONE)
			SDL_RenderCopyEx(m_Renderer, m_Texture, &n_rcTexture, &m_Rect,
				m_dRotation, &m_RotateCenter, m_RendererFlip);
		else
			SDL_RenderCopy(m_Renderer, m_Texture, &n_rcTexture, &m_Rect);
	}

	void FSdl::VideoProc()
	{
		if (!m_Window || m_eStatus != ESdlStatus::SS_Play) return;

		auto Data = m_cb.Execute(AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (!Data.Frame) return;

		UpdateVideo(Data.Frame);

		ProgressBarProc(Data.dProgress);
		SDL_RenderPresent(m_Renderer);
	}

	void FSdl::AudioCallback(void* n_UserData, 
		unsigned char* n_szStream, int n_nLen)
	{
		FSdl* Sdl = (FSdl*)n_UserData;
		if (Sdl) Sdl->AudioProc(n_szStream, n_nLen);
	}

	void FSdl::AudioProc(unsigned char* n_szStream, int n_nLen)
	{
		auto Data = m_cb.Execute(AVMediaType::AVMEDIA_TYPE_AUDIO);
		if (!Data.Frame) return;

		UpdateAudio(Data.Frame, n_szStream, n_nLen);
	}

	void FSdl::ProgressBarProc(double n_dProgress)
	{
		if (n_dProgress <= 0) return;

		int nWidth = static_cast<int>(m_Rect.w * 0.9f);
		int nHeight = 10;
		int nLeft = (m_Rect.w - nWidth) / 2;
		int nTop = m_Rect.h - nHeight - 20;

		// Draw background process bar
		SDL_Rect BgRect = { nLeft, nTop, nWidth, nHeight };
		SDL_SetRenderDrawColor(m_Renderer, 255, 255, 255, 255);
		SDL_RenderFillRect(m_Renderer, &BgRect);

		// ¼ÆËã½ø¶ÈÌõ¿í¶È²¢»æÖÆ½ø¶ÈÌõ
		auto barWidth = static_cast<int>(n_dProgress * BgRect.w);
		SDL_Rect BarRect = { BgRect.x, BgRect.y, barWidth, BgRect.h };
		SDL_SetRenderDrawColor(m_Renderer, 0, 255, 0, 255);
		SDL_RenderFillRect(m_Renderer, &BarRect);
	}

	void FSdl::UpdateVideo(AVFrame* n_Frame)
	{
		if (!n_Frame || !m_Texture || !m_Renderer ||
			m_eStatus != ESdlStatus::SS_Play) return;

		SDL_LockMutex(m_Mutex);

		auto ret = 0;
		auto ePixFmt = (AVPixelFormat)n_Frame->format;

		SDL_Rect rcFrame = { 0, 0, n_Frame->width, n_Frame->height };

		if (ePixFmt == AVPixelFormat::AV_PIX_FMT_NV12 ||
			ePixFmt == AVPixelFormat::AV_PIX_FMT_NV21)
			ret = SDL_UpdateNVTexture(m_Texture, &rcFrame,
				n_Frame->data[0], n_Frame->linesize[0],
				n_Frame->data[1], n_Frame->linesize[1]);
		else
			ret = SDL_UpdateYUVTexture(m_Texture, &rcFrame,
				n_Frame->data[0], n_Frame->linesize[0],
				n_Frame->data[1], n_Frame->linesize[1],
				n_Frame->data[2], n_Frame->linesize[2]);

		if (ret == 0)
		{
			SDL_RenderClear(m_Renderer);
			RendererCopy(rcFrame);
			//SDL_RenderPresent(m_Renderer);
		}
		else
			AVDebug("Warning: Fail to render frame on the window,Error: %s\n",
				SDL_GetError());

		SDL_UnlockMutex(m_Mutex);
	}

	void FSdl::UpdateAudio(AVFrame* n_Frame,
		unsigned char* n_Stream, int n_nLen) const
	{
		if (m_eStatus != ESdlStatus::SS_Play || !n_Frame) return;

		SDL_memset(n_Stream, 0, n_nLen);

		if (m_nPlanar == 0)
		{
			//SDL_MixAudio(n_Stream, n_Frame->data[0], n_nLen, SDL_MIX_MAXVOLUME);
			memcpy_s(n_Stream, n_nLen, n_Frame->data[0], 
				(rsize_t)(n_Frame->nb_samples) * n_Frame->ch_layout.nb_channels * m_nBytesPerSample);
		}
		else
		{
			auto p = n_Stream;

			for (int i = 0; i < n_Frame->nb_samples; i++)
			{
				for (int j = 0; j < n_Frame->ch_layout.nb_channels; j++)
				{
					memcpy_s(p, m_nBytesPerSample,
						&n_Frame->data[j][m_nBytesPerSample * i], m_nBytesPerSample);

					p += m_nBytesPerSample;
				}
			}
		}
	}

}

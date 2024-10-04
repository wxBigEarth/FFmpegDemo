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

	void FSdl::Init(const unsigned char n_nMediaMask, 
		std::shared_ptr<ISdlHandle> n_Handle)
	{
		ThrowExceptionExpr(!n_Handle, "ISdlHandle could not be null\n");

		uint32_t nFlags = 0;

		if (IsCompriseMedia(n_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
			nFlags |= SDL_INIT_VIDEO;
		if (IsCompriseMedia(n_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
			nFlags |= SDL_INIT_AUDIO;

		int ret = SDL_Init(nFlags);
		ThrowExceptionExpr(ret, 
			"Fail to initialize SDL: %s.\n", SDL_GetError());

		m_SdlHandle = n_Handle;
		m_nMediaMask = n_nMediaMask;

		// Callback function to update video
		auto pfn = std::bind(&FSdl::Update, this,
			std::placeholders::_1, std::placeholders::_2);
		m_SdlHandle->SetUpdateCallback(pfn);
	}

	void FSdl::InitVideo(const char* n_szTitle,
		const int n_nWidth, const int n_nHeight,
		unsigned int n_nPixFmt /*= SDL_PIXELFORMAT_IYUV*/)
	{
		if (m_Window) return;

		bool b = IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (!b) return;

		m_Window = SDL_CreateWindow(n_szTitle,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			n_nWidth,
			n_nHeight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

		ThrowExceptionExpr(!m_Window, 
			"Fail to create window: %s.\n", SDL_GetError());

		CreateRenderer(m_Window);
		CreateTexture(n_nPixFmt);
		
		m_nDisplayEvent = SDL_RegisterEvents(1);
	}

	void FSdl::InitVideo(const void* n_WinId,
		unsigned int n_nPixFmt /*= SDL_PIXELFORMAT_IYUV*/)
	{
		if (m_Window) return;

		bool b = IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (!b) return;

		m_Window = SDL_CreateWindowFrom(n_WinId);

		ThrowExceptionExpr(!m_Window, 
			"Fail to create window: %s.\n", SDL_GetError());

		CreateRenderer(m_Window);
		CreateTexture(n_nPixFmt);

		m_nDisplayEvent = SDL_RegisterEvents(1);
	}

	void FSdl::InitAudio(
		int n_nSampleRate,
		int n_nFrameSize,
		int n_nNbChannel,
		AVSampleFormat n_nSampleFormat)
	{
		bool b = IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO);
		if (!b) return;

		SDL_AudioSpec AudioSpec = { 0 };

		AudioSpec.freq = n_nSampleRate;
		AudioSpec.channels = n_nNbChannel;
		AudioSpec.silence = 0;
		AudioSpec.samples = n_nFrameSize;
		AudioSpec.callback = AudioCallback;
		AudioSpec.userdata = this;

		switch (n_nSampleFormat)
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
				av_get_sample_fmt_name(n_nSampleFormat));
			break;
		}

		int ret = SDL_OpenAudio(&AudioSpec, nullptr);
		ThrowExceptionExpr(ret, "Fail to open audio: %s.\n", SDL_GetError());

		m_nChannels = AudioSpec.channels;
		m_nPlanar = av_sample_fmt_is_planar(n_nSampleFormat);
		m_nBytesPerSample = av_get_bytes_per_sample(n_nSampleFormat);;

		Play();
	}

	const unsigned int FSdl::Event()
	{
		if (!m_Window) return 0;

		SDL_Event Event;

		while (SDL_PollEvent(&Event))
		{
			switch (Event.type) {
			case SDL_QUIT:
				if (m_SdlHandle) m_SdlHandle->SDL_Stop();
				Stop();
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
				if (Event.window.event == 0x0005) /* WM_SIZE */
				{
					SDL_GetWindowSize(m_Window, &m_Rect.w, &m_Rect.h);
				}
				break;
			default:
				if (m_nDisplayEvent == Event.type && m_Window)
					VideoProc();
				break;
			}

			SDL_Delay(10);
		}

		return Event.type;
	}

	void FSdl::SendDisplayEvent() const
	{
		SDL_Event Event{};
		Event.type = m_nDisplayEvent;
		SDL_PushEvent(&Event);
	}

	void FSdl::Update(double n_dCur, double n_dMax)
	{
		VideoProc();
		ProgressBarProc(n_dCur, n_dMax);
		SDL_RenderPresent(m_Renderer);
	}

	void FSdl::Play()
	{
		m_eStatus = ESdlStatus::SS_Play;
		SDL_PauseAudio(0);
	}

	void FSdl::Pause()
	{
		m_eStatus = ESdlStatus::SS_Pause;
		SDL_PauseAudio(0);
	}

	bool FSdl::IsPause() const
	{
		return m_eStatus == ESdlStatus::SS_Pause;
	}

	void FSdl::Stop()
	{
		m_eStatus = ESdlStatus::SS_Stop;
	}

	void FSdl::Release()
	{
		m_eStatus = ESdlStatus::SS_Stop;

		if (m_Texture) SDL_DestroyTexture(m_Texture);
		if (m_Renderer) SDL_DestroyRenderer(m_Renderer);
		if (m_Window) SDL_DestroyWindow(m_Window);
		SDL_CloseAudio();
		SDL_Quit();

		m_Texture = nullptr;
		m_Renderer = nullptr;
		memset(&m_Rect, 0, sizeof(SDL_Rect));
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
	}

	void FSdl::CreateTexture(unsigned int n_nPixFmt)
	{
		SDL_GetWindowSize(m_Window, &m_Rect.w, &m_Rect.h);

		m_Texture = SDL_CreateTexture(m_Renderer,
			n_nPixFmt,
			SDL_TEXTUREACCESS_STREAMING,
			m_Rect.w,
			m_Rect.h);
	}

	void FSdl::CreateRenderer(SDL_Window* n_Window)
	{
		m_Renderer = SDL_CreateRenderer(n_Window, -1, 0);
		ThrowExceptionExpr(!m_Renderer,
			"Fail to create renderer: %s.\n", SDL_GetError());
	}

	void FSdl::VideoProc()
	{
		if (!m_SdlHandle || !m_Window) return;

		auto Frame = m_SdlHandle->SDL_ReadFrame(AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (!Frame) return;

		UpdateVideo(Frame);
		m_SdlHandle->SDL_ReadEnd(Frame);
	}

	void FSdl::AudioCallback(void* n_UserData, 
		unsigned char* n_szStream, int n_nLen)
	{
		FSdl* Sdl = (FSdl*)n_UserData;
		if (Sdl) Sdl->AudioProc(n_szStream, n_nLen);
	}

	void FSdl::AudioProc(unsigned char* n_szStream, int n_nLen)
	{
		if (!m_SdlHandle) return;

		auto Frame = m_SdlHandle->SDL_ReadFrame(AVMediaType::AVMEDIA_TYPE_AUDIO);
		if (!Frame) return;

		UpdateAudio(Frame, n_szStream, n_nLen);
		m_SdlHandle->SDL_ReadEnd(Frame);
	}

	void FSdl::ProgressBarProc(double n_dCur, double n_dMax)
	{
		if (n_dMax == 0) return;

		int nWidth = static_cast<int>(m_Rect.w * 0.9f);
		int nHeight = 10;
		int nLeft = (m_Rect.w - nWidth) / 2;
		int nTop = m_Rect.h - nHeight - 20;

		// Draw background process bar
		SDL_Rect BgRect = { nLeft, nTop, nWidth, nHeight };
		SDL_SetRenderDrawColor(m_Renderer, 255, 255, 255, 255);
		SDL_RenderFillRect(m_Renderer, &BgRect);

		// 计算进度条宽度并绘制进度条
		auto dProgress = n_dCur / n_dMax;
		auto barWidth = static_cast<int>(dProgress * BgRect.w);
		SDL_Rect BarRect = { BgRect.x, BgRect.y, barWidth, BgRect.h };
		SDL_SetRenderDrawColor(m_Renderer, 0, 255, 0, 255);
		SDL_RenderFillRect(m_Renderer, &BarRect);
	}

	void FSdl::UpdateVideo(AVFrame* n_Frame)
	{
		if (!n_Frame || !m_Texture || !m_Renderer ||
			m_eStatus != ESdlStatus::SS_Play) return;

		auto ret = 0;
		auto ePixFmt = (AVPixelFormat)n_Frame->format;

		if (ePixFmt == AVPixelFormat::AV_PIX_FMT_NV12 ||
			ePixFmt == AVPixelFormat::AV_PIX_FMT_NV21)
			ret = SDL_UpdateNVTexture(m_Texture, &m_Rect,
				n_Frame->data[0], n_Frame->linesize[0],
				n_Frame->data[1], n_Frame->linesize[1]);
		else
			ret = SDL_UpdateYUVTexture(m_Texture, &m_Rect,
				n_Frame->data[0], n_Frame->linesize[0],
				n_Frame->data[1], n_Frame->linesize[1],
				n_Frame->data[2], n_Frame->linesize[2]);

		if (ret == 0)
		{
			SDL_RenderClear(m_Renderer);
			SDL_RenderCopy(m_Renderer, m_Texture, nullptr, &m_Rect);
			//SDL_RenderPresent(m_Renderer);
		}
		else
			AVDebug("Warning: Fail to render frame on the window,Error: %s\n",
				SDL_GetError());
	}

	void FSdl::UpdateAudio(AVFrame* n_Frame,
		unsigned char* n_Stream, int n_nLen) const
	{
		if (m_eStatus != ESdlStatus::SS_Play || !n_Frame) return;

		SDL_memset(n_Stream, 0, n_nLen);

		std::string sSamples;

		sSamples.resize(n_nLen);

		if (m_nPlanar == 0)
		{
			memcpy_s((char*)sSamples.c_str(), n_nLen, n_Frame->data[0], n_nLen);
		}
		else
		{
			int k = 0;
			for (int i = 0; i < n_Frame->nb_samples; i++)
			{
				for (int j = 0; j < m_nChannels; j++)
				{
					memcpy_s((char*)sSamples.c_str() + k, m_nBytesPerSample,
						&n_Frame->data[j][m_nBytesPerSample * i], m_nBytesPerSample);

					k += m_nBytesPerSample;
				}
			}
		}

		SDL_MixAudio(n_Stream, (const Uint8*)sSamples.data(), n_nLen, SDL_MIX_MAXVOLUME);
	}

}

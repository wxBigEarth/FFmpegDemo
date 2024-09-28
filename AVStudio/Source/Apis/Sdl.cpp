#include "Apis/Sdl.h"
#include "Util/Debug.h"
#include "Util/MediaMask.h"


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
		if (IsCompriseMedia(n_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
			nFlags |= SDL_INIT_AUDIO;

		int ret = SDL_Init(nFlags);
		ThrowExceptionExpr(ret, "Fail to initialize SDL: %s.\n", SDL_GetError());
	}

	void FSdl::InitVideo(const char* n_szTitle, 
		const int n_nWidth, const int n_nHeight)
	{
		if (m_Window) return;

		m_Window = SDL_CreateWindow(n_szTitle,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			n_nWidth,
			n_nHeight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

		ThrowExceptionExpr(!m_Window, 
			"Fail to create window: %s.\n", SDL_GetError());

		CreateRenderer(m_Window);

		CreateTexture(SDL_PIXELFORMAT_IYUV);
	}

	void FSdl::InitVideo(const void* n_WinId)
	{
		if (m_Window) return;

		m_Window = SDL_CreateWindowFrom(n_WinId);

		ThrowExceptionExpr(!m_Window, 
			"Fail to create window: %s.\n", SDL_GetError());

		CreateRenderer(m_Window);

		CreateTexture(SDL_PIXELFORMAT_IYUV);
	}

	void FSdl::InitAudio(AVCodecContext* n_CodecContext, 
		SDL_AudioCallback n_Cb, void* n_Param)
	{
		SDL_AudioSpec AudioSpec = { 0 };

		AudioSpec.freq = n_CodecContext->sample_rate;
		AudioSpec.channels = n_CodecContext->ch_layout.nb_channels;
		AudioSpec.silence = 0;
		AudioSpec.samples = n_CodecContext->frame_size;
		AudioSpec.callback = n_Cb;
		AudioSpec.userdata = n_Param;

		switch (n_CodecContext->sample_fmt)
		{
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
			break;
		}

		int ret = SDL_OpenAudio(&AudioSpec, nullptr);
		ThrowExceptionExpr(ret, "Fail to open audio: %s.\n", SDL_GetError());

		m_nPlanar = av_sample_fmt_is_planar(n_CodecContext->sample_fmt) + 1;

		// 计算输出的缓冲区大小
		//out_buffer_size = av_samples_get_buffer_size(
		// NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

		Play();
	}

	void FSdl::UpdateYUV(AVFrame* n_Frame)
	{
		if (!m_Texture || !m_Renderer || m_nPause || !n_Frame) return;

		SDL_UpdateYUVTexture(m_Texture, &m_Rect,
			n_Frame->data[0], n_Frame->linesize[0],
			n_Frame->data[1], n_Frame->linesize[1],
			n_Frame->data[2], n_Frame->linesize[2]);

		SDL_RenderClear(m_Renderer);
		SDL_RenderCopy(m_Renderer, m_Texture, nullptr, &m_Rect);
		SDL_RenderPresent(m_Renderer);
	}

	void FSdl::UpdateAudio(AVFrame* n_Frame, 
		unsigned char* n_Stream, int n_nLen) const
	{
		if (m_nPause || !n_Frame) return;

		SDL_memset(n_Stream, 0, n_nLen);

		int nOffset = n_Frame->linesize[0] / m_nPlanar;

		for (size_t i = 0; i < m_nPlanar; i++)
			SDL_MixAudio(n_Stream + nOffset * i, n_Frame->data[i], nOffset, SDL_MIX_MAXVOLUME);
	}

	const unsigned int FSdl::Event()
	{
		SDL_Event Event;
		//SDL_PollEvent(&m_Event);

		//switch (m_Event.type) {
		//case SDL_QUIT:
		//	break;
		//case SDL_KEYUP:
		//	if (m_Event.key.keysym.sym == SDLK_SPACE)
		//	{
		//		if (IsPause())
		//			Play();
		//		else
		//			Pause();
		//	}
		//	break;
		//default:
		//	break;
		//}

		SDL_PumpEvents();

		if (SDL_PeepEvents(&Event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1)
		{
			switch (Event.type)
			{
			case SDL_QUIT:
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
			default:
				break;
			}
		}

		return Event.type;
	}

	void FSdl::Play()
	{
		m_nPause = 0;
		SDL_PauseAudio(m_nPause);
	}

	void FSdl::Pause()
	{
		m_nPause = 1;
		SDL_PauseAudio(m_nPause);
	}

	bool FSdl::IsPause() const
	{
		return m_nPause == 1;
	}

	void FSdl::Release()
	{
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

}

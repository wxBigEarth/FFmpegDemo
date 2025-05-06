#include "Core/WorkShop.h"
#include "Util/Debug.h"


namespace avstudio
{
	FWorkShop::~FWorkShop()
	{
		Release();
	}

	void FWorkShop::Init(ECtxType n_eCtxType, 
		const unsigned char n_nMediaMask /*= MEDIAMASK_AV*/, 
		std::shared_ptr<FSetting> n_Setting /*= nullptr*/)
	{
		m_eCtxType = n_eCtxType;
		m_nMediaMask = 0;
		m_Setting = n_Setting;

		if (!Fmt.IsValid())
		{
			// Recording PCM data
			if (m_eCtxType == ECtxType::CT_Input)
				m_nMediaMask = n_nMediaMask;
			return;
		}

		// Make sure it's input context
		if (m_eCtxType == ECtxType::CT_Input)
		{
			if (IsCompriseMedia(n_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
			{
				VideoParts.nStreamIndex = Fmt.FindStreamIndex(
					AVMediaType::AVMEDIA_TYPE_VIDEO, nullptr);
				if (VideoParts.nStreamIndex >= 0)
				{
					VideoParts.Stream = Fmt.FindStream(VideoParts.nStreamIndex);
					// Maybe it has config hardware acceleration
					VideoParts.Codec = FCodecContext::BuildDecodeCodec(
						VideoParts.Stream->codecpar->codec_id, m_Setting);
					VideoParts.Codec->CopyCodecParameter(VideoParts.Stream);
					VideoParts.Codec->ConfigureHwAccel();

					CombineMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO);
				}
			}

			if (IsCompriseMedia(n_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
			{
				const AVCodec* AudioCodec = nullptr;
				AudioParts.nStreamIndex = Fmt.FindStreamIndex(
					AVMediaType::AVMEDIA_TYPE_AUDIO, &AudioCodec);
				if (AudioParts.nStreamIndex >= 0)
				{
					AudioParts.Stream = Fmt.FindStream(AudioParts.nStreamIndex);
					AudioParts.Codec = FCodecContext::BuildCodec(AudioCodec, m_Setting);
					AudioParts.Codec->CopyCodecParameter(AudioParts.Stream);

					CombineMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO);
				}
			}
		}
		else if (m_eCtxType == ECtxType::CT_Output)
		{
			m_nMediaMask = 0;
		}
	}

	void FWorkShop::Release()
	{
		VideoParts.Release();
		AudioParts.Release();
		m_vFragments.clear();

		m_nMediaMask = 0;
		m_nGroupId = 0;

		Fmt.Release();
	}

	void FWorkShop::Processing()
	{
		if (IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (VideoParts.Codec && VideoParts.Codec->Context)
			{
				double a = VideoParts.Codec->Context->time_base.den * 1.0 /
					VideoParts.Codec->Context->time_base.num /
					VideoParts.Codec->Context->framerate.num *
					VideoParts.Codec->Context->framerate.den;
				VideoParts.Duration = (int64_t)(a);
			}
			
			if (m_eCtxType == ECtxType::CT_Input &&
				VideoParts.nStreamIndex >= 0)
			{
				ThrowExceptionExpr((IsValid() && !VideoParts.Stream) ||
					!VideoParts.Codec ||
					!VideoParts.Codec->Context,
					"Video stream: not initialize complete\n");
			}
		}
		
		if (IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (AudioParts.Codec && AudioParts.Codec->Context)
			{
				auto a = AudioParts.Codec->Context->time_base.den * 1.0 /
					(double)AudioParts.Codec->Context->sample_rate *
					AudioParts.Codec->Context->frame_size /
					AudioParts.Codec->Context->time_base.num;
				AudioParts.Duration = (int64_t)(a);
			}
			
			if (m_eCtxType == ECtxType::CT_Input &&
				AudioParts.nStreamIndex >= 0)
			{
				ThrowExceptionExpr((IsValid() && !AudioParts.Stream) ||
					!AudioParts.Codec ||
					!AudioParts.Codec->Context,
					"Audio stream: not initialize complete\n");
			}
		}

		if (m_vFragments.size() > 0)
		{
			Fmt.SeekFrame(-1,
				(int64_t)(m_vFragments[m_nFragmentIndex].Start * AV_TIME_BASE),
				AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
		}
	}

	void FWorkShop::SetGroupId(const unsigned char n_nId)
	{
		m_nGroupId = n_nId;
	}

	const unsigned char FWorkShop::GetGroupId() const
	{
		return m_nGroupId;
	}

	void FWorkShop::SetGroupIndex(const unsigned char n_nIndex)
	{
		m_nGroupIndex = n_nIndex;
	}

	const unsigned char FWorkShop::GetGroupIndex() const
	{
		return m_nGroupIndex;
	}

	const bool FWorkShop::IsValid() const
	{
		return Fmt.IsValid();
	}

	const unsigned char FWorkShop::GetMediaMask() const
	{
		return m_nMediaMask;
	}

	AVFormatContext* FWorkShop::FormatContext()
	{
		return Fmt.Context;
	}

	AVCodecID FWorkShop::GetCodecId(AVMediaType n_eMediaType)
	{
		AVCodecID eCodecId = AVCodecID::AV_CODEC_ID_NONE;
		if (m_eCtxType != ECtxType::CT_Output) return eCodecId;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			// Check desire codec id
			if (VideoParts.DesireCodecId != AVCodecID::AV_CODEC_ID_NONE)
			{
				auto Codec = FindEncodeCodec(
					VideoParts.DesireCodecId, m_Setting);
				// If VideoParts.DesireCodecId is not video codec, reset it
				if (!Codec || Codec->type != AVMediaType::AVMEDIA_TYPE_VIDEO)
					VideoParts.DesireCodecId = AVCodecID::AV_CODEC_ID_NONE;
			}

			if (VideoParts.DesireCodecId != AVCodecID::AV_CODEC_ID_NONE)
				eCodecId = VideoParts.DesireCodecId;
			else if (IsValid() && Fmt.Context->oformat)
				eCodecId = Fmt.Context->oformat->video_codec;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			// Check desire codec id
			if (AudioParts.DesireCodecId != AVCodecID::AV_CODEC_ID_NONE)
			{
				auto Codec = FindEncodeCodec(
					AudioParts.DesireCodecId, m_Setting);
				// If VideoParts.DesireCodecId is not audio codec, reset it
				if (Codec->type != AVMediaType::AVMEDIA_TYPE_AUDIO)
					AudioParts.DesireCodecId = AVCodecID::AV_CODEC_ID_NONE;
			}

			if (AudioParts.DesireCodecId != AVCodecID::AV_CODEC_ID_NONE)
				eCodecId = AudioParts.DesireCodecId;
			else if (IsValid() && Fmt.Context->oformat)
				eCodecId = Fmt.Context->oformat->audio_codec;
		}

		return eCodecId;
	}

	void FWorkShop::EnableStream(AVMediaType n_eMediaType, bool n_bSelect)
	{
		if (n_bSelect)
		{
			CombineMedia(m_nMediaMask, n_eMediaType);
		}
		else
		{
			SeparateMedia(m_nMediaMask, n_eMediaType);
		}
	}

	bool FWorkShop::CheckMedia(AVMediaType n_eMediaType) const
	{
		return IsCompriseMedia(m_nMediaMask, n_eMediaType);
	}

	std::shared_ptr<FCodecContext> FWorkShop::BuildCodecContext(AVStream* n_Stream)
	{
		std::shared_ptr<FCodecContext> Result = nullptr;

		if (!n_Stream) return Result;

		// Make sure it's output context
		if (m_eCtxType != ECtxType::CT_Output) return Result;

		auto eCodecId = n_Stream->codecpar->codec_id;
		auto Codec = FindEncodeCodec(eCodecId, m_Setting);
		if (!Codec) return Result;
		
		if (!IsCompriseMedia(m_nMediaMask, Codec->type)) return Result;

		if (n_Stream->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			Result = VideoParts.Codec;
			if (!Result)
			{
				VideoParts.Codec = FCodecContext::BuildCodec(Codec, m_Setting);
				VideoParts.Codec->CopyCodecParameter(n_Stream);
				
				auto ctx = VideoParts.Codec->Context;
				Result = VideoParts.Codec;

				ctx->framerate = n_Stream->r_frame_rate;
				ctx->time_base = n_Stream->time_base;
				ctx->gop_size = 12;
			}
			
			// if ctx->framerate.num too large, first frame will be gray
			if (ctx->framerate.num > 60)
				ctx->framerate = AVRational{ 25, 1 };

			// enable show Thumbnail
			if (IsValid() &&
				Fmt.Context->oformat &&
				(Fmt.Context->oformat->flags & AVFMT_GLOBALHEADER))
				ctx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;

			if (m_funcMiddleware) m_funcMiddleware(Result->Context);
			VideoParts.Codec->ConfigureHwAccel();
		}
		else if (n_Stream->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			Result = AudioParts.Codec;
			if (!Result)
			{
				AudioParts.Codec = FCodecContext::BuildCodec(Codec, m_Setting);
				AudioParts.Codec->CopyCodecParameter(n_Stream);
				Result = AudioParts.Codec;
			}

			if (m_funcMiddleware) m_funcMiddleware(Result->Context);
		}

		return Result;
	}

	std::shared_ptr<FCodecContext> FWorkShop::BuildCodecContext(
		AVCodecID n_eCodecID,
		AVCodecContext* n_InputCodecContext)
	{
		std::shared_ptr<FCodecContext> Result = nullptr;

		// Make sure it's output context
		if (m_eCtxType != ECtxType::CT_Output) return Result;

		auto Codec = FindEncodeCodec(n_eCodecID, m_Setting);
		if (!Codec) return Result;

		if (!IsCompriseMedia(m_nMediaMask, Codec->type)) return Result;

		if (Codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			Result = AudioParts.Codec;
			if (!Result)
			{
				AudioParts.Codec = FCodecContext::BuildCodec(Codec, m_Setting);
				auto ctx = AudioParts.Codec->Context;
				Result = AudioParts.Codec;

				if (n_InputCodecContext)
				{
					ctx->bit_rate = n_InputCodecContext->bit_rate;
					ctx->sample_rate = GetSupportedSampleRate(Codec, 
						n_InputCodecContext->sample_rate);
					ctx->sample_fmt = GetSupportedSampleFormat(Codec, 
						n_InputCodecContext->sample_fmt);
					GetSupportedChannelLayout(Codec, &ctx->ch_layout);
					ctx->time_base = { 1, ctx->sample_rate };
					// For empty m_Context, copy the frame_size
					if (!Fmt.IsValid()) ctx->frame_size = 
						n_InputCodecContext->frame_size;
				}
			}

			if (m_funcMiddleware) m_funcMiddleware(Result->Context);
			CodecContextAddition(ctx);
		}
		else if (Codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			Result = VideoParts.Codec;
			if (!Result)
			{
				VideoParts.Codec = FCodecContext::BuildCodec(Codec, m_Setting);
				auto ctx = VideoParts.Codec->Context;
				Result = VideoParts.Codec;

				if (n_InputCodecContext)
				{
					ctx->bit_rate = n_InputCodecContext->bit_rate;
					ctx->width = n_InputCodecContext->width;
					ctx->height = n_InputCodecContext->height;
					ctx->framerate = GetSupportedFrameRate(Codec,
						n_InputCodecContext->framerate);
					ctx->time_base = av_inv_q(ctx->framerate);
					ctx->pix_fmt = GetSupportedPixelFormat(Codec, 
						n_InputCodecContext->pix_fmt);
					ctx->gop_size = n_InputCodecContext->gop_size;
				}
			}

			// if ctx->framerate.num too large, first frame will be gray
			if (ctx->framerate.num > 60)
				ctx->framerate = AVRational{ 25, 1 };

			// enable show Thumbnail
			if (IsValid() && 
				Fmt.Context->oformat &&
				(Fmt.Context->oformat->flags & AVFMT_GLOBALHEADER))
				ctx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;

			if (m_funcMiddleware) m_funcMiddleware(Result->Context);

			CodecContextAddition(ctx);
			VideoParts.Codec->ConfigureHwAccel();
		}

		return Result;
	}

	std::shared_ptr<FCodecContext> FWorkShop::BuildDefaultCodecContext(
		AVMediaType n_eMediaType,
		AVCodecContext* n_InputCodecContext /*= nullptr*/)
	{
		std::shared_ptr<FCodecContext> Result = nullptr;

		if (!IsValid()) return Result;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			Result = BuildCodecContext(
				Fmt.Context->oformat->video_codec, n_InputCodecContext);
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			Result = BuildCodecContext(
				Fmt.Context->oformat->audio_codec, n_InputCodecContext);

		return Result;
	}

	AVStream* FWorkShop::FindStream(AVMediaType n_eMediaType)
	{
		AVStream* Stream = nullptr;
		if (!IsCompriseMedia(m_nMediaMask, n_eMediaType)) return Stream;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (VideoParts.nStreamIndex < 0)
			{
				VideoParts.nStreamIndex = Fmt.FindStreamIndex(n_eMediaType);
				VideoParts.Stream = Fmt.FindStream(VideoParts.nStreamIndex);
			}

			Stream = VideoParts.Stream;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (AudioParts.nStreamIndex < 0)
			{
				AudioParts.nStreamIndex = Fmt.FindStreamIndex(n_eMediaType);
				AudioParts.Stream = Fmt.FindStream(AudioParts.nStreamIndex);
			}

			Stream = AudioParts.Stream;
		}

		return Stream;
	}

	void FWorkShop::BuildStream(std::shared_ptr<FWorkShop> n_Input,
		AVMediaType n_eMediaType /*= AVMediaType::AVMEDIA_TYPE_UNKNOWN*/)
	{
		if (!n_Input) return;
		if (m_eCtxType != ECtxType::CT_Output) return;

		if (n_Input->VideoParts.Stream && 
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			VideoParts.Stream = Fmt.BuildStream(n_Input->VideoParts.Stream);
			VideoParts.nStreamIndex = VideoParts.Stream->index;
		}

		if (n_Input->AudioParts.Stream && 
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			AudioParts.Stream = Fmt.BuildStream(n_Input->AudioParts.Stream);
			AudioParts.nStreamIndex = AudioParts.Stream->index;
		}
	}

	void FWorkShop::BuildStream(AVCodecContext* n_CodecContext, 
		AVMediaType n_eMediaType /*= AVMediaType::AVMEDIA_TYPE_UNKNOWN*/)
	{
		if (m_eCtxType != ECtxType::CT_Output) return;
		if (!IsCompriseMedia(m_nMediaMask, n_eMediaType)) return;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			VideoParts.Stream = Fmt.BuildStream(n_CodecContext);
			VideoParts.nStreamIndex = VideoParts.Stream->index;
		}

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			AudioParts.Stream = Fmt.BuildStream(n_CodecContext);
			AudioParts.nStreamIndex = AudioParts.Stream->index;
		}
	}

	void FWorkShop::OpenCodecContext(AVMediaType n_eMediaType)
	{
		if (!IsValid()) return;

		if (VideoParts.Codec && 
			(VideoParts.nFlag || m_eCtxType == ECtxType::CT_Output) && 
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN))
		{
			VideoParts.Codec->Open();
		}

		if (AudioParts.Codec && 
			(AudioParts.nFlag || m_eCtxType == ECtxType::CT_Output) &&
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN))
		{
			AudioParts.Codec->Open();
		}
	}

	void FWorkShop::CreateFrameConverter(std::shared_ptr<FWorkShop> n_Output,
		AVMediaType n_eMediaType /*= AVMediaType::AVMEDIA_TYPE_UNKNOWN*/)
	{
		if (!n_Output) return;
		if (m_eCtxType != ECtxType::CT_Input) return;

		if (n_Output->VideoParts.Codec &&
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (VideoParts.nFlag <= 0)
				VideoParts.nFlag = CompareCodecFormat(
					VideoParts.Codec, n_Output->VideoParts.Codec);
			if (VideoParts.nFlag > 0 && !VideoParts.Sws)
			{
				VideoParts.Sws = std::make_shared<FSwsScale>();
				VideoParts.Sws->Alloc(VideoParts.Codec->Context->width,
					VideoParts.Codec->Context->height,
					VideoParts.Codec->GetPixelFormat(),
					n_Output->VideoParts.Codec->Context->width,
					n_Output->VideoParts.Codec->Context->height,
					n_Output->VideoParts.Codec->GetPixelFormat());
				if (!VideoParts.Sws->Context) VideoParts.Sws.reset();
			}
		}

		if (n_Output->AudioParts.Codec &&
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (AudioParts.nFlag <= 0)
				AudioParts.nFlag = CompareCodecFormat(
					AudioParts.Codec, n_Output->AudioParts.Codec);
			if (AudioParts.nFlag > 0 && !AudioParts.Resample)
			{
				AudioParts.Resample = std::make_shared<FResample>();
				AudioParts.Resample->Alloc(AudioParts.Codec->Context,
					n_Output->AudioParts.Codec->Context);
			}

			CreateAudioFifo(n_Output->AudioParts.Codec);
		}
	}

	void FWorkShop::SetDecodeFlag(int n_nFlag,
		AVMediaType n_eMediaType /*= AVMediaType::AVMEDIA_TYPE_UNKNOWN*/)
	{
		if (m_eCtxType != ECtxType::CT_Input) return;

		if ((n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO ||
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (VideoParts.Codec)
			{
				VideoParts.nFlag = n_nFlag;
				if (VideoParts.nFlag == 0)
					VideoParts.Codec.reset();
			}
		}

		if ((n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsCompriseMedia(m_nMediaMask, AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (AudioParts.Codec)
			{
				AudioParts.nFlag = n_nFlag;
				if (AudioParts.nFlag == 0)
					AudioParts.Codec.reset();
			}
		}
	}

	void FWorkShop::SetupFilter(AVMediaType n_eMediaType, 
		std::shared_ptr<IFilter> n_Filter)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			VideoParts.Filter = n_Filter;
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			AudioParts.Filter = n_Filter;
	}

	void FWorkShop::CreateAudioFifo(std::shared_ptr<FCodecContext> n_AudioCodec)
	{
		if (!AudioParts.Codec || 
			!AudioParts.Codec->Context ||
			!n_AudioCodec) return;

		if (AudioParts.Codec->Context->frame_size != n_AudioCodec->Context->frame_size)
		{
			if (!AudioParts.FiFo) AudioParts.FiFo = std::make_shared<FAudioFifo>();
			if (!AudioParts.FiFo->Context)
				AudioParts.FiFo->Alloc(
					n_AudioCodec->Context->sample_fmt,
					n_AudioCodec->Context->ch_layout.nb_channels,
					n_AudioCodec->Context->frame_size);
		}
	}

	int64_t FWorkShop::AdjustPts(int64_t n_nPts, AVMediaType n_eMediaType)
	{
		int64_t nResult = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			nResult = VideoParts.FramePts;
			VideoParts.FramePts += n_nPts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			nResult = AudioParts.FramePts;
			AudioParts.FramePts += n_nPts;
		}

		return nResult;
	}

	void FWorkShop::AddLastPts(int64_t n_nPts, AVMediaType n_eMediaType)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			VideoParts.FramePts += n_nPts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			AudioParts.FramePts += n_nPts;
		}
	}

	int64_t FWorkShop::GetLastPts(AVMediaType n_eMediaType) const
	{
		int64_t nResult = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			nResult = VideoParts.FramePts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			nResult = AudioParts.FramePts;
		}

		return nResult;
	}

	void FWorkShop::PickupFragment(const double n_dStart, const double n_dLength)
	{
		if (m_eCtxType != ECtxType::CT_Input || 
			n_dStart < 0 ||
			n_dLength <= 0) return;

		FFragment Section;
		Section.Start = n_dStart;
		Section.vOk = !VideoParts.Stream;
		Section.aOk = !AudioParts.Stream;

		double dTo = n_dStart + n_dLength;
		if (VideoParts.Stream)
			Section.vTo = (int64_t)(dTo / av_q2d(VideoParts.Stream->time_base));

		if (AudioParts.Stream)
			Section.aTo = (int64_t)(dTo / av_q2d(AudioParts.Stream->time_base));

		m_vFragments.emplace_back(Section);
	}

	int64_t FWorkShop::TryPickup(int64_t n_nPts, AVMediaType n_eMediaType)
	{
		int64_t nResult = 0;
		int nSize = (int)m_vFragments.size();
		if (nSize == 0) return nResult;
		if (m_nFragmentIndex >= nSize) return AVERROR_EOF;

		nResult = -1;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO && VideoParts.Stream)
		{
			if (n_nPts >= m_vFragments[m_nFragmentIndex].vTo)
				m_vFragments[m_nFragmentIndex].vOk = true;
			else nResult = VideoParts.PacketPts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO && AudioParts.Stream)
		{
			if (n_nPts >= m_vFragments[m_nFragmentIndex].aTo)
				m_vFragments[m_nFragmentIndex].aOk = true;
			else nResult = AudioParts.PacketPts;
		}

		if (m_vFragments[m_nFragmentIndex].IsOk())
		{
			m_nFragmentIndex++;
			if (m_nFragmentIndex < nSize)
			{
				Fmt.SeekFrame(-1,
					(int64_t)(m_vFragments[m_nFragmentIndex].Start * AV_TIME_BASE),
					AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
			}
			else nResult = AVERROR_EOF;
		}

		return nResult;
	}

	void FWorkShop::SetupMiddleware(
		std::function<void(AVCodecContext*)> n_func)
	{
		m_funcMiddleware = n_func;
	}

}

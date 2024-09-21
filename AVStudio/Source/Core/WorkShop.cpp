#include "Core/WorkShop.h"
#include "Util/Debug.h"


namespace avstudio
{
	static void* AVClone(EDataType n_eDataType, void* n_Data)
	{
		void* Data = nullptr;

		if (!n_Data) return Data;

		if (n_eDataType == EDataType::DT_Frame)
			Data = av_frame_clone((AVFrame*)n_Data);
		else if (n_eDataType == EDataType::DT_Packet)
			Data = av_packet_clone((AVPacket*)n_Data);

		return Data;
	}

	FWorkShop::~FWorkShop()
	{
		Release();
	}

	void FWorkShop::Init(ECtxType n_eCtxType, const unsigned int n_nStreamMask)
	{
		m_eCtxType = n_eCtxType;
		m_nStreamMask = n_nStreamMask;

		if (!Fmt.IsValid()) return;

		// Make sure it's input context
		if (m_eCtxType == ECtxType::CT_Input)
		{
			if (IsStreamSelected(AVMediaType::AVMEDIA_TYPE_VIDEO))
			{
				const AVCodec* VideoCodec = nullptr;
				VideoParts.nStreamIndex = Fmt.FindStreamIndex(
					AVMediaType::AVMEDIA_TYPE_VIDEO, &VideoCodec);
				if (VideoParts.nStreamIndex >= 0)
				{
					VideoParts.Stream = Fmt.FindStream(VideoParts.nStreamIndex);
					VideoParts.Codec = new FCodecContext();

					if (GetSetting()->bEnableHwAccel)
						VideoCodec = FCodecContext::FindDecodeCodec(VideoCodec->id);

					VideoParts.Codec->Alloc(VideoCodec);
					VideoParts.Codec->CopyCodecParameter(VideoParts.Stream);
				}
			}

			if (IsStreamSelected(AVMediaType::AVMEDIA_TYPE_AUDIO))
			{
				const AVCodec* AudioCodec = nullptr;
				AudioParts.nStreamIndex = Fmt.FindStreamIndex(
					AVMediaType::AVMEDIA_TYPE_AUDIO, &AudioCodec);
				if (AudioParts.nStreamIndex >= 0)
				{
					AudioParts.Stream = Fmt.FindStream(AudioParts.nStreamIndex);
					AudioParts.Codec = new FCodecContext();

					AudioParts.Codec->Alloc(AudioCodec);
					AudioParts.Codec->CopyCodecParameter(AudioParts.Stream);
				}
			}
		}
		else if (m_eCtxType == ECtxType::CT_Output)
		{
			m_nStreamMask = 0;
		}
	}

	void FWorkShop::Release()
	{
		VideoParts.Release();
		AudioParts.Release();
		m_vSectons.clear();

		m_nStreamMask = -1;
		m_nGroupId = -1;
		m_nVideoPts = 0;
		m_nAudioPts = 0;

		Fmt.Release();
		IOHandle = nullptr;
	}

	void FWorkShop::Processing()
	{
		if (IsStreamSelected(AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (m_eCtxType == ECtxType::CT_Output)
			{
				if (VideoParts.Codec && VideoParts.Codec->Context)
				{
					double a = VideoParts.Codec->Context->time_base.den /
						VideoParts.Codec->Context->time_base.num /
						VideoParts.Codec->Context->framerate.num *
						VideoParts.Codec->Context->framerate.den;
					VideoParts.Duration = (int64_t)(a);
				}
			}
			else if (m_eCtxType == ECtxType::CT_Input)
			{
				ThrowExceptionExpr((IsValid() && !VideoParts.Stream) ||
					!VideoParts.Codec ||
					!VideoParts.Codec->Context,
					"Video stream: not initialize complete\n");
			}
		}
		
		if (IsStreamSelected(AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (m_eCtxType == ECtxType::CT_Output)
			{
				if (AudioParts.Codec && AudioParts.Codec->Context)
				{
					auto a = AudioParts.Codec->Context->time_base.den /
						(double)AudioParts.Codec->Context->sample_rate *
						AudioParts.Codec->Context->frame_size /
						AudioParts.Codec->Context->time_base.num;
					AudioParts.Duration = (int64_t)(a);
				}
			}
			else if (m_eCtxType == ECtxType::CT_Input)
			{
				ThrowExceptionExpr((IsValid() && !AudioParts.Stream) ||
					!AudioParts.Codec ||
					!AudioParts.Codec->Context,
					"Audio stream: not initialize complete\n");
			}
		}
	}

	void FWorkShop::SetGroupId(const unsigned int n_nId)
	{
		m_nGroupId = n_nId;
	}

	const unsigned int FWorkShop::GetGroupId() const
	{
		return m_nGroupId;
	}

	void FWorkShop::SetGroupIndex(const unsigned int n_nIndex)
	{
		m_nGroupIndex = n_nIndex;
	}

	const unsigned int FWorkShop::GetGroupIndex() const
	{
		return m_nGroupIndex;
	}

	const bool FWorkShop::IsValid() const
	{
		return Fmt.IsValid();
	}

	AVFormatContext* FWorkShop::FormatContext()
	{
		return Fmt.Context;
	}

	AVCodecID FWorkShop::GetCodecId(AVMediaType n_eMediaType)
	{
		AVCodecID eCodecId = AVCodecID::AV_CODEC_ID_NONE;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			// Check desire codec id
			if (VideoParts.DesireCodecId != AVCodecID::AV_CODEC_ID_NONE)
			{
				auto Codec = FCodecContext::FindEncodeCodec(
					VideoParts.DesireCodecId);
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
				auto Codec = FCodecContext::FindEncodeCodec(
					AudioParts.DesireCodecId);
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
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (n_bSelect)
			{
				m_nStreamMask |= (1 << n_eMediaType);
				if (!VideoParts.Buffer &&
					m_eCtxType == ECtxType::CT_Output &&
					IsValid())
					VideoParts.Buffer = new Queue<void*>();
			}
			else
			{
				m_nStreamMask &= ~(1 << n_eMediaType);
				VideoParts.ReleaseBuffer();
			}

			VideoParts.bIsEnd = !n_bSelect;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (n_bSelect)
			{
				m_nStreamMask |= (1 << n_eMediaType);
				if (!AudioParts.Buffer &&
					m_eCtxType == ECtxType::CT_Output &&
					IsValid())
					AudioParts.Buffer = new Queue<void*>();
			}
			else
			{
				m_nStreamMask &= ~(1 << n_eMediaType);
				AudioParts.ReleaseBuffer();
			}

			AudioParts.bIsEnd = !n_bSelect;
		}
	}

	bool FWorkShop::IsStreamSelected(AVMediaType n_eMediaType) const
	{
		if (n_eMediaType > AVMediaType::AVMEDIA_TYPE_UNKNOWN && 
			n_eMediaType < AVMediaType::AVMEDIA_TYPE_NB)
			return m_nStreamMask & (1 << n_eMediaType);

		return false;
	}

	void FWorkShop::ResetEndFlag()
	{
		if (IsStreamSelected(AVMediaType::AVMEDIA_TYPE_VIDEO))
			VideoParts.bIsEnd = false;
		if (IsStreamSelected(AVMediaType::AVMEDIA_TYPE_AUDIO))
			AudioParts.bIsEnd = false;
	}

	FCodecContext* FWorkShop::BuildCodecContext(AVStream* n_Stream)
	{
		FCodecContext* Result = nullptr;

		if (!n_Stream) return Result;

		// Make sure it's output context
		if (m_eCtxType != ECtxType::CT_Output) return Result;

		auto eCodecId = n_Stream->codecpar->codec_id;
		auto Codec = FCodecContext::FindEncodeCodec(eCodecId);
		if (!Codec) return Result;

		if (n_Stream->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO && 
			IsStreamSelected(AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			VideoParts.Codec = new FCodecContext();
			auto ctx = VideoParts.Codec->Alloc(Codec);
			VideoParts.Codec->CopyCodecParameter(n_Stream);

			ctx->framerate = n_Stream->r_frame_rate;
			// if ctx->framerate.num too large, first frame will be gray
			if (ctx->framerate.num > 60)
				ctx->framerate = AVRational{ 25, 1 };

			ctx->time_base = n_Stream->time_base;
			ctx->gop_size = 12;

			// enable show Thumbnail
			if (IsValid() &&
				Fmt.Context->oformat && 
				(Fmt.Context->oformat->flags & AVFMT_GLOBALHEADER))
				ctx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;

			Result = VideoParts.Codec;

			if (m_funcMiddleware) m_funcMiddleware(this);
		}
		else if (n_Stream->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO && 
			IsStreamSelected(AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			AudioParts.Codec = new FCodecContext();
			AudioParts.Codec->Alloc(Codec);
			AudioParts.Codec->CopyCodecParameter(n_Stream);

			Result = AudioParts.Codec;

			if (m_funcMiddleware) m_funcMiddleware(this);
		}

		return Result;
	}

	FCodecContext* FWorkShop::BuildCodecContext(AVCodecID n_eCodecID, 
		AVCodecContext* n_InputCodecContext)
	{
		FCodecContext* Result = nullptr;

		// Make sure it's output context
		if (m_eCtxType != ECtxType::CT_Output) return Result;

		auto Codec = FCodecContext::FindEncodeCodec(n_eCodecID);
		if (!Codec) return Result;

		if (Codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO && 
			IsStreamSelected(Codec->type))
		{
			Result = AudioParts.Codec;
			if (!Result)
			{
				AudioParts.Codec = new FCodecContext();
				auto ctx = AudioParts.Codec->Alloc(Codec);
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

				CodecContextAddition(ctx);
			}

			if (m_funcMiddleware) m_funcMiddleware(this);
		}
		else if (Codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO && 
			IsStreamSelected(Codec->type))
		{
			Result = VideoParts.Codec;
			if (!Result)
			{
				VideoParts.Codec = new FCodecContext();
				auto ctx = VideoParts.Codec->Alloc(Codec);
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

				// if ctx->framerate.num too large, first frame will be gray
				if (ctx->framerate.num > 60)
					ctx->framerate = AVRational{ 25, 1 };

				// enable show Thumbnail
				if (IsValid() && 
					Fmt.Context->oformat &&
					(Fmt.Context->oformat->flags & AVFMT_GLOBALHEADER))
					ctx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;

				CodecContextAddition(ctx);
			}

			if (m_funcMiddleware) m_funcMiddleware(this);
		}

		return Result;
	}

	FCodecContext* FWorkShop::BuildDefaultCodecContext(AVMediaType n_eMediaType, 
		AVCodecContext* n_InputCodecContext /*= nullptr*/)
	{
		FCodecContext* Result = nullptr;

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

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO &&
			IsStreamSelected(n_eMediaType))
		{
			if (VideoParts.nStreamIndex < 0)
			{
				VideoParts.nStreamIndex = Fmt.FindStreamIndex(n_eMediaType);
				VideoParts.Stream = Fmt.FindStream(VideoParts.nStreamIndex);
			}

			Stream = VideoParts.Stream;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO &&
			IsStreamSelected(n_eMediaType))
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

	void FWorkShop::BuildStream(FWorkShop* n_Input,
		AVMediaType n_eMediaType /*= AVMediaType::AVMEDIA_TYPE_UNKNOWN*/)
	{
		if (!n_Input) return;
		if (m_eCtxType != ECtxType::CT_Output) return;

		if (n_Input->VideoParts.Stream && 
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO && 
			IsStreamSelected(n_eMediaType))
		{
			VideoParts.Stream = Fmt.BuildStream(n_Input->VideoParts.Stream);
			VideoParts.nStreamIndex = VideoParts.Stream->index;
		}

		if (n_Input->AudioParts.Stream && 
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO && 
			IsStreamSelected(n_eMediaType))
		{
			AudioParts.Stream = Fmt.BuildStream(n_Input->AudioParts.Stream);
			AudioParts.nStreamIndex = AudioParts.Stream->index;
		}
	}

	void FWorkShop::BuildStream(AVCodecContext* n_CodecContext, 
		AVMediaType n_eMediaType /*= AVMediaType::AVMEDIA_TYPE_UNKNOWN*/)
	{
		if (m_eCtxType != ECtxType::CT_Output) return;

		if ((n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO ||
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) && 
			IsStreamSelected(n_eMediaType))
		{
			VideoParts.Stream = Fmt.BuildStream(n_CodecContext);
			VideoParts.nStreamIndex = VideoParts.Stream->index;
		}

		if ((n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) && 
			IsStreamSelected(n_eMediaType))
		{
			AudioParts.Stream = Fmt.BuildStream(n_CodecContext);
			AudioParts.nStreamIndex = AudioParts.Stream->index;
		}
	}

	void FWorkShop::OpenCodecContext(AVMediaType n_eMediaType)
	{
		if (VideoParts.Codec && 
			(VideoParts.nShouldDecode || m_eCtxType == ECtxType::CT_Output) && 
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN))
		{
			VideoParts.Codec->Open();
		}

		if (AudioParts.Codec && 
			(AudioParts.nShouldDecode || m_eCtxType == ECtxType::CT_Output) &&
			(n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
				n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN))
		{
			AudioParts.Codec->Open();
		}
	}

	void FWorkShop::CheckForDecoding(FWorkShop* n_Output, 
		AVMediaType n_eMediaType /*= AVMediaType::AVMEDIA_TYPE_UNKNOWN*/)
	{
		if ((n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO ||
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsStreamSelected(AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (VideoParts.nShouldDecode <= 0)
				VideoParts.nShouldDecode = CompareCodecFormat(
					VideoParts.Codec, n_Output->VideoParts.Codec);
			if (VideoParts.nShouldDecode > 0 &&
				!VideoParts.Sws)
			{
				VideoParts.Sws = new FSwsScale();
				VideoParts.Sws->Alloc(VideoParts.Codec->Context->width,
					VideoParts.Codec->Context->height,
					VideoParts.Codec->GetPixelFormat(),
					n_Output->VideoParts.Codec->Context->width,
					n_Output->VideoParts.Codec->Context->height,
					n_Output->VideoParts.Codec->GetPixelFormat());
			}
		}

		if ((n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsStreamSelected(AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (AudioParts.nShouldDecode <= 0)
				AudioParts.nShouldDecode = CompareCodecFormat(
					AudioParts.Codec, n_Output->AudioParts.Codec);
			if (AudioParts.nShouldDecode > 0 &&
				!AudioParts.Resample)
			{
				AudioParts.Resample = new FResample();
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
			IsStreamSelected(AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (VideoParts.Codec)
			{
				VideoParts.nShouldDecode = n_nFlag;

				if (VideoParts.nShouldDecode == 0)
				{
					VideoParts.Codec->Release();
					delete VideoParts.Codec;
					VideoParts.Codec = nullptr;
				}
			}
		}

		if ((n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO ||
			n_eMediaType == AVMediaType::AVMEDIA_TYPE_UNKNOWN) &&
			IsStreamSelected(AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (AudioParts.Codec)
			{
				AudioParts.nShouldDecode = n_nFlag;

				if (AudioParts.nShouldDecode == 0)
				{
					AudioParts.Codec->Release();
					delete AudioParts.Codec;
					AudioParts.Codec = nullptr;
				}
			}
		}
	}

	void FWorkShop::SetupFilter(AVMediaType n_eMediaType, IFilter* n_Filter)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			VideoParts.Filter = n_Filter;
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			AudioParts.Filter = n_Filter;
	}

	void FWorkShop::CreateAudioFifo(FCodecContext* n_AudioCodec)
	{
		if (!AudioParts.Codec || !AudioParts.Codec->Context) return;

		if (AudioParts.Codec->Context->frame_size != n_AudioCodec->Context->frame_size)
		{
			if (!AudioParts.FiFo) AudioParts.FiFo = new FAudioFifo();
			if (!AudioParts.FiFo->Context)
				AudioParts.FiFo->Alloc(
					n_AudioCodec->Context->sample_fmt,
					n_AudioCodec->Context->ch_layout.nb_channels,
					n_AudioCodec->Context->frame_size);
		}
	}

	void FWorkShop::PushData(AVMediaType n_eMediaType, EDataType n_eDataType, void* n_Data)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (VideoParts.Buffer)
			{
				FDataItem* DataItem = new FDataItem();
				DataItem->DataType = n_eDataType;
				DataItem->MediaType = n_eMediaType;
				DataItem->Data = AVClone(n_eDataType, n_Data);

				VideoParts.Buffer->Push(DataItem);
			}
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (AudioParts.Buffer)
			{
				FDataItem* DataItem = new FDataItem();
				DataItem->DataType = n_eDataType;
				DataItem->MediaType = n_eMediaType;
				DataItem->Data = AVClone(n_eDataType, n_Data);

				AudioParts.Buffer->Push(DataItem);
			}
		}

		if (IOHandle)
		{
			void* Data = AVClone(n_eDataType, n_Data);
			IOHandle->WriteData(n_eMediaType, n_eDataType, Data);
		}

		GenerateFile();
	}

	int FWorkShop::PopData(AVMediaType n_eMediaType, FDataItem*& n_DataItem)
	{
		int ret = -1;
		void* DataItem = nullptr;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (VideoParts.Buffer && VideoParts.Buffer->Size() > 0)
			{
				ret = VideoParts.Buffer->Pop(DataItem, 10);
				if (ret >= 0)
				{
					n_DataItem = (FDataItem*)DataItem;
				}
			}
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (AudioParts.Buffer && AudioParts.Buffer->Size() > 0)
			{
				ret = AudioParts.Buffer->Pop(DataItem, 10);
				if (ret >= 0)
				{
					n_DataItem = (FDataItem*)DataItem;
				}
			}
		}

		return ret;
	}

	int FWorkShop::FrontData(AVMediaType n_eMediaType, FDataItem*& n_DataItem)
	{
		int ret = -1;
		void* DataItem = nullptr;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (VideoParts.Buffer)
			{
				ret = VideoParts.Buffer->Front(DataItem);
				if (ret >= 0)
				{
					n_DataItem = (FDataItem*)DataItem;
				}
			}
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (AudioParts.Buffer)
			{
				ret = AudioParts.Buffer->Front(DataItem);
				if (ret >= 0)
				{
					n_DataItem = (FDataItem*)DataItem;
				}
			}
		}

		return ret;
	}

	size_t FWorkShop::GetBufferSize(AVMediaType n_eMediaType)
	{
		size_t nResult = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (!IsStreamSelected(n_eMediaType))
				nResult = AV_TIME_BASE;
			else if (VideoParts.Buffer)
				nResult = VideoParts.Buffer->Size();
			else if (IOHandle)
				nResult = IOHandle->GetBufferSize(n_eMediaType);
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (!IsStreamSelected(n_eMediaType))
				nResult = AV_TIME_BASE;
			else if (AudioParts.Buffer)
				nResult = AudioParts.Buffer->Size();
			else if (IOHandle)
				nResult = IOHandle->GetBufferSize(n_eMediaType);
		}

		return nResult;
	}

	void FWorkShop::GenerateFile()
	{
		if (m_eCtxType != ECtxType::CT_Output) return;

		int vRet = 0;
		int aRet = 0;

		while ((VideoParts.Buffer && VideoParts.Buffer->Size() > 0) ||
			(AudioParts.Buffer && AudioParts.Buffer->Size() > 0))
		{
			if (!VideoParts.bIsEnd && !m_vDataItem)
			{
				vRet = PopData(AVMediaType::AVMEDIA_TYPE_VIDEO, m_vDataItem);
				if (vRet >= 0)
				{
					VideoParts.bIsEnd = !m_vDataItem ||
						!m_vDataItem->Data ||
						m_vDataItem->DataType != EDataType::DT_Packet;
				}
			}

			if (!AudioParts.bIsEnd && !m_aDataItem)
			{
				aRet = PopData(AVMediaType::AVMEDIA_TYPE_AUDIO, m_aDataItem);
				if (aRet >= 0)
				{
					AudioParts.bIsEnd = !m_aDataItem ||
						!m_aDataItem->Data ||
						m_aDataItem->DataType != EDataType::DT_Packet;
				}
			}

			if (VideoParts.bIsEnd || AudioParts.bIsEnd)
			{
				if (VideoParts.bIsEnd)
				{
					AVFreeData(&m_vDataItem);
					VideoParts.CleanBuffer();

					if (!AudioParts.bIsEnd && m_aDataItem)
					{
						Fmt.InterleavedWritePacket(m_aDataItem->p());
						AVFreeData(&m_aDataItem);
					}
				}
				
				if (AudioParts.bIsEnd)
				{
					AVFreeData(&m_aDataItem);
					AudioParts.CleanBuffer();

					if (!VideoParts.bIsEnd && m_vDataItem)
					{
						Fmt.InterleavedWritePacket(m_vDataItem->p());
						AVFreeData(&m_vDataItem);
					}
				}
			}
			else if (m_vDataItem && m_aDataItem)
			{
				AVPacket* v = m_vDataItem->p();
				AVPacket* a = m_aDataItem->p();

				if (av_compare_ts(v->pts, v->time_base,
					a->pts, a->time_base) < 0)
				{
					Fmt.InterleavedWritePacket(v);
					AVFreeData(&m_vDataItem);
				}
				else
				{
					Fmt.InterleavedWritePacket(a);
					AVFreeData(&m_aDataItem);
				}
			}
			else
			{
				break;
			}
		}
	}

	int64_t FWorkShop::AdjustPts(int64_t n_nPts, AVMediaType n_eMediaType)
	{
		int64_t nResult = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			nResult = m_nVideoPts;
			m_nVideoPts += n_nPts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			nResult = m_nAudioPts;
			m_nAudioPts += n_nPts;
		}

		return nResult;
	}

	void FWorkShop::AddLastPts(int64_t n_nPts, AVMediaType n_eMediaType)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			m_nVideoPts += n_nPts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			m_nAudioPts += n_nPts;
		}
	}

	int64_t FWorkShop::GetLastPts(AVMediaType n_eMediaType) const
	{
		int64_t nResult = 0;

		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			nResult = m_nVideoPts;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			nResult = m_nAudioPts;
		}

		return nResult;
	}

	void FWorkShop::PickupFragment(const double n_dStart, const double n_dLength)
	{
		if (m_eCtxType != ECtxType::CT_Input) return;

		FSection Section;
		double dTo = n_dStart + n_dLength;

		if (VideoParts.Stream)
		{
			Section.vFrom = (int64_t)(n_dStart / av_q2d(VideoParts.Stream->time_base));
			Section.vTo = (int64_t)(dTo / av_q2d(VideoParts.Stream->time_base));
		}

		if (AudioParts.Stream)
		{
			Section.aFrom = (int64_t)(n_dStart / av_q2d(AudioParts.Stream->time_base));
			Section.aTo = (int64_t)(dTo / av_q2d(AudioParts.Stream->time_base));
		}

		m_vSectons.emplace_back(Section);
	}

	int64_t FWorkShop::TryPickup(int64_t n_nPts, AVMediaType n_eMediaType)
	{
		int64_t nResult = 0;
		if (m_vSectons.size() > 0) nResult = -1;

		int64_t n = 0;

		for (size_t i = 0; i < m_vSectons.size(); i++)
		{
			if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
			{
				if (i == 0)
					n = m_vSectons[i].vFrom;
				else
					n += m_vSectons[i].vFrom - m_vSectons[i - 1].vFrom;

				if (m_vSectons[i].vFrom <= n_nPts && 
					m_vSectons[i].vTo > n_nPts)
				{
					nResult = n;
					break;
				}

				if (m_vSectons[i].vTo < n_nPts &&
					i == m_vSectons.size() - 1)
				{
					nResult = AVERROR_EOF;
					break;
				}
			}
			else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
			{
				if (i == 0)
					n = m_vSectons[i].aFrom;
				else
					n += m_vSectons[i].aFrom - m_vSectons[i - 1].aFrom;

				if (m_vSectons[i].aFrom <= n_nPts && 
					m_vSectons[i].aTo > n_nPts)
				{
					nResult = n;
					break;
				}

				if (m_vSectons[i].aTo < n_nPts &&
					i == m_vSectons.size() - 1)
				{
					nResult = AVERROR_EOF;
					break;
				}
			}
		}

		return nResult;
	}

	void FWorkShop::SetupMiddleware(std::function<void(FWorkShop*)> n_func)
	{
		m_funcMiddleware = n_func;
	}

}
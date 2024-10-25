#include "Apis/CodecContext.h"
#include "Util/Debug.h"


namespace avstudio
{
	static AVPixelFormat GetHwCodecFormat(AVCodecContext* n_CodecContext,
		const AVPixelFormat* n_PixelFormats)
	{
		const AVPixelFormat* p = n_PixelFormats;
		auto ePixelFormat = (AVPixelFormat)(uint64_t)n_CodecContext->opaque;

		for (; *p != AVPixelFormat::AV_PIX_FMT_NONE; p++)
		{
			if (*p == ePixelFormat)
				return *p;
		}

		return AVPixelFormat::AV_PIX_FMT_NONE;
	}

	FCodecContext::~FCodecContext()
	{
		Release();
	}

	std::shared_ptr<FCodecContext> FCodecContext::BuildDecodeCodec(
		AVCodecID n_CodecId, std::shared_ptr<FSetting> n_Setting /*= nullptr*/)
	{
		auto Codec = FindDecodeCodec(n_CodecId, n_Setting);
		return BuildCodec(Codec, n_Setting);
	}

	std::shared_ptr<FCodecContext> FCodecContext::BuildEncodeCodec(
		AVCodecID n_CodecId, std::shared_ptr<FSetting> n_Setting /*= nullptr*/)
	{
		auto Codec = FindEncodeCodec(n_CodecId, n_Setting);
		return BuildCodec(Codec, n_Setting);
	}

	std::shared_ptr<FCodecContext> FCodecContext::BuildCodec(
		const AVCodec* n_Codec, std::shared_ptr<FSetting> n_Setting /*= nullptr*/)
	{
		auto CodecContext = std::make_shared<FCodecContext>();
		CodecContext->Alloc(n_Codec, n_Setting);
		return CodecContext;
	}

	AVCodecContext* FCodecContext::Alloc(const AVCodec* n_Codec,
		std::shared_ptr<FSetting> n_Setting)
	{
		ThrowExceptionExpr(!n_Codec, 
			"Invalid parameter: n_Codec is nullptr.\n");

		Context = avcodec_alloc_context3(n_Codec);
		ThrowExceptionExpr(!Context, 
			"Fail to alloc codec context: %s", n_Codec->name);

		m_Setting = n_Setting;

		return Context;
	}

	void FCodecContext::Release()
	{
		if (Context)
			avcodec_free_context(&Context);

		if (m_HwDeviceContext)
			av_buffer_unref(&m_HwDeviceContext);

		if (m_Packet) av_packet_free(&m_Packet);
		if (m_Frame) av_frame_free(&m_Frame);
		if (m_DestFrame) av_frame_free(&m_DestFrame);
	}

	void FCodecContext::CopyCodecParameter(const AVStream* n_Stream)
	{
		ThrowExceptionExpr(!n_Stream, "Invalid parameter: n_Stream is nullptr.\n");

		int ret = avcodec_parameters_to_context(Context, n_Stream->codecpar);
		ThrowExceptionCodeExpr(ret < 0, ret, 
			"Fail to copy codec parameters form stream.");
		
		Context->time_base = n_Stream->time_base;
		Context->pkt_timebase = n_Stream->time_base;
		Context->framerate = n_Stream->r_frame_rate;
	}

	void FCodecContext::CopyCodecParameter(const AVCodecContext* n_CodecContext,
		std::shared_ptr<FSetting> n_Setting)
	{
		Alloc(n_CodecContext->codec, n_Setting);

		AVCodecParameters* par = avcodec_parameters_alloc();
		avcodec_parameters_from_context(par, n_CodecContext);

		avcodec_parameters_to_context(Context, par);
		avcodec_parameters_free(&par);

		Context->time_base = n_CodecContext->time_base;
		Context->framerate = n_CodecContext->framerate;
	}

	void FCodecContext::ConfigureHwAccel()
	{
		if (m_Setting && m_Setting->bEnableHwAccel &&
			Context->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO &&
			IsHardwareCodec())
		{
			auto eCodecType = Type();
			unsigned int nFlag = AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX;

			if (eCodecType == ECodecType::CT_Encoder)
				nFlag = AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX;

			auto HwConfig = GetHwCodecConfig(Context->codec,
				m_Setting->GetHwDeviceType(), nFlag);

			if (HwConfig)
			{
				CreateHwContext(m_Setting->GetHwDeviceType(), HwConfig);

				if (eCodecType == ECodecType::CT_Decoder)
				{
					Context->thread_count = 0;
					Context->get_format = GetHwCodecFormat;
				}
			}
		}
	}

	void FCodecContext::Open(AVDictionary** n_Options)
	{
		ThrowExceptionExpr(!Context, "You should call function Alloc() first.\n");

		if (IsOpen()) return;

		int ret = avcodec_open2(Context, Context->codec, n_Options);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to open codec context.");
	}

	const bool FCodecContext::IsOpen() const
	{
		if (!Context) return false;
		return avcodec_is_open(Context);
	}

	int FCodecContext::DecodePacket(const AVPacket* n_Packet,
		std::function<int(AVFrame* n_Frame)> n_Func)
	{
		ThrowExceptionExpr(!Context, "You should call function Alloc() first.\n");

		int ret = avcodec_send_packet(Context, n_Packet);
		if (ret < 0) return ret;

		AVFrame* Frame = nullptr;

		while (ret >= 0)
		{
			if (!m_Frame) m_Frame = av_frame_alloc();
			ret = avcodec_receive_frame(Context, m_Frame);
			if (ret == AVERROR(EAGAIN))
				break;
			else if (ret == AVERROR_EOF)
			{
				if (n_Func) n_Func(nullptr);
				break;
			}

			ThrowExceptionExpr(ret < 0, "Error during decoding: %s\n",
				ErrorCode2String(ret).c_str());

			Frame = m_Frame;
			if (Context->hw_device_ctx && 
				m_Frame && m_Frame->format == GetHwPixelFormat())
			{
				if (!m_DestFrame) m_DestFrame = av_frame_alloc();
				m_DestFrame->format = m_eSwPixelFormat;

				/* Retrieve data from GPU to CPU */
				ret = av_hwframe_transfer_data(m_DestFrame, m_Frame, 0);
				ThrowExceptionExpr(ret < 0 || !m_DestFrame, 
					"Error during decoding: %s\n",
					ErrorCode2String(ret).c_str());

				m_DestFrame->pts = m_Frame->pts;
				m_DestFrame->pkt_dts = m_Frame->pkt_dts;
				m_DestFrame->best_effort_timestamp = m_Frame->best_effort_timestamp;

				Frame = m_DestFrame;
			}

			if (n_Func) n_Func(Frame);
			if (Frame == m_DestFrame) av_frame_unref(Frame);
		}

		return ret;
	}

	int FCodecContext::EncodeFrame(const AVFrame* n_Frame, 
		std::function<int(AVPacket* n_Packet)> n_Func)
	{
		ThrowExceptionExpr(!Context, 
			"You should call function Alloc() first.\n");

		int ret = 0;
		AVFrame* Frame = (AVFrame*)n_Frame;

		if (Context->hw_frames_ctx && 
			Frame && Frame->format == m_eSwPixelFormat)
		{
			if (!m_DestFrame) m_DestFrame = av_frame_alloc();

			ret = av_hwframe_get_buffer(Context->hw_frames_ctx, m_DestFrame, 0);
			ThrowExceptionExpr(ret < 0 || !m_DestFrame->hw_frames_ctx,
				"Fail to get buffer for HW frame, Error: %s\n",
				ErrorCode2String(ret).c_str());

			/* Retrieve data from CPU to GPU */
			ret = av_hwframe_transfer_data(m_DestFrame, Frame, 0);
			ThrowExceptionExpr(ret < 0 || !m_DestFrame,
				"Error during decoding: %s\n",
				ErrorCode2String(ret).c_str());

			m_DestFrame->pts = Frame->pts;
			m_DestFrame->pkt_dts = Frame->pkt_dts;
			m_DestFrame->best_effort_timestamp = Frame->best_effort_timestamp;

			Frame = m_DestFrame;
		}

		ret = avcodec_send_frame(Context, Frame);
		if (Frame == m_DestFrame) av_frame_unref(Frame);
		if (ret < 0) return ret;

		while (ret >= 0)
		{
			if (!m_Packet) m_Packet = av_packet_alloc();
			ret = avcodec_receive_packet(Context, m_Packet);
			if (ret == AVERROR(EAGAIN))
				break;
			else if (ret == AVERROR_EOF)
			{
				if (n_Func) n_Func(nullptr);
				break;
			}

			ThrowExceptionExpr(ret < 0, "Error during encoding: %s\n",
				ErrorCode2String(ret).c_str());

			if (n_Func) n_Func(m_Packet);
			av_packet_unref(m_Packet);
		}

		return ret;
	}

	const ECodecType FCodecContext::Type() const
	{
		if (!Context) return ECodecType::CT_None;

		if (av_codec_is_decoder(Context->codec))
			return ECodecType::CT_Decoder;

		if (av_codec_is_encoder(Context->codec))
			return ECodecType::CT_Encoder;

		return ECodecType::CT_None;
	}

	const bool FCodecContext::IsHardwareCodec() const
	{
		if (!Context) return false;
		return Context->codec->capabilities & AV_CODEC_CAP_HARDWARE;
	}

	int FCodecContext::GetPixFmtPlaneCount() const
	{
		if (Context->codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO)
			return av_pix_fmt_count_planes(Context->pix_fmt);

		return 0;
	}

	int FCodecContext::IsSampleFmtPlanar() const
	{
		if (Context->codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO)
			return av_sample_fmt_is_planar(Context->sample_fmt);

		return 0;
	}

	int FCodecContext::GetBytesPerSample() const
	{
		if (Context->codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO)
			return av_get_bytes_per_sample(Context->sample_fmt);

		return 0;
	}

	const AVPixelFormat FCodecContext::GetHwPixelFormat() const
	{
		if (!Context || !Context->opaque || !IsHardwareCodec())
			return AVPixelFormat::AV_PIX_FMT_NONE;
		return (AVPixelFormat)(uint64_t)Context->opaque;
	}

	const AVPixelFormat FCodecContext::GetPixelFormat() const
	{
		if (!Context) return AVPixelFormat::AV_PIX_FMT_NONE;
		if (m_eSwPixelFormat == AVPixelFormat::AV_PIX_FMT_NONE)
			return Context->pix_fmt;

		return m_eSwPixelFormat;
	}

	const AVCodecHWConfig* FCodecContext::GetHwCodecConfig(
		const AVCodec* n_Codec,
		AVHWDeviceType n_eHwDeviceType,
		unsigned int n_nFlag)
	{
		const AVCodecHWConfig* HwConfig = nullptr;

		for (int i = 0; n_Codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO; i++)
		{
			auto Config = avcodec_get_hw_config(n_Codec, i);
			if (!Config) break;

			if (Config->methods & n_nFlag &&
				Config->device_type == n_eHwDeviceType)
			{
				//n_ePixelFormat = Config->pix_fmt;
				HwConfig = Config;
				break;
			}
		}

		return HwConfig;
	}

	void FCodecContext::CreateHwContext(AVHWDeviceType n_eHwDeviceType,
		const AVCodecHWConfig* n_HwConfig)
	{
		int ret = av_hwdevice_ctx_create(&m_HwDeviceContext, n_eHwDeviceType,
			nullptr, nullptr, 0);

		ThrowExceptionCodeExpr(ret < 0, ret, "Failed to create specified HW device.\n");

		GetValidSwFormat(m_HwDeviceContext, n_HwConfig);

		if (av_codec_is_decoder(Context->codec))
			Context->hw_device_ctx = av_buffer_ref(m_HwDeviceContext);
		else if (av_codec_is_encoder(Context->codec))
		{
			int err = 0;

			auto hw_frames_ref = av_hwframe_ctx_alloc(m_HwDeviceContext);
			ThrowExceptionExpr(!hw_frames_ref,
				"Failed to create %s frame context.\n",
				av_hwdevice_get_type_name(n_eHwDeviceType));

			Context->pix_fmt = GetHwPixelFormat();

			auto frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
			frames_ctx->format = Context->pix_fmt;
			frames_ctx->sw_format = m_eSwPixelFormat;
			frames_ctx->width = Context->width;
			frames_ctx->height = Context->height;
			frames_ctx->initial_pool_size = 20;

			err = av_hwframe_ctx_init(hw_frames_ref);
			if (err < 0) av_buffer_unref(&hw_frames_ref);

			ThrowExceptionExpr(err < 0,
				"Failed to initialize %s frame context, error: %s\n",
				av_hwdevice_get_type_name(n_eHwDeviceType),
				ErrorCode2String(err).c_str());

			Context->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
			av_buffer_unref(&hw_frames_ref);
		}
	}

	void FCodecContext::GetValidSwFormat(AVBufferRef* n_HwDeviceContext,
		const AVCodecHWConfig* n_HwConfig)
	{
		if (!n_HwConfig || !Context) return;

		auto Constraints = av_hwdevice_get_hwframe_constraints(
			n_HwDeviceContext, n_HwConfig);
		if (!Constraints) return;

		Context->opaque = (void*)(*Constraints->valid_hw_formats);
		m_eSwPixelFormat = *(Constraints->valid_sw_formats);

#if 0
		auto p = Constraints->valid_hw_formats;
		while (*p != AVPixelFormat::AV_PIX_FMT_NONE)
			AVDebug("Hw Fromats: %s\n", av_get_pix_fmt_name(*p++));

		p = Constraints->valid_sw_formats;
		while (*p != AVPixelFormat::AV_PIX_FMT_NONE)
			AVDebug("Sw Fromats: %s\n", av_get_pix_fmt_name(*p++));
#endif

		av_hwframe_constraints_free(&Constraints);
	}

	//////////////////////////////////////////////////////////////////////
	const AVCodec* FindDecodeCodec(AVCodecID n_CodecID,
		std::shared_ptr<FSetting> n_Setting)
	{
		const AVCodec* Codec = nullptr;

		if (n_Setting && n_Setting->bEnableHwAccel)
		{
			auto vHwCodecs = FindAllHwCodecs();
			for (size_t i = 0; i < vHwCodecs.size(); i++)
			{
				if (vHwCodecs[i].CodecType == ECodecType::CT_Decoder &&
					vHwCodecs[i].GraphicCard == n_Setting->GetGraphicCard() &&
					vHwCodecs[i].Codec->id == n_CodecID)
				{
					Codec = vHwCodecs[i].Codec;
					break;
				}
			}
		}

		if (!Codec) Codec = avcodec_find_decoder(n_CodecID);
		if (!Codec)
		{
			AVDebug("Fail to find decoder: %s",
				avcodec_get_name(n_CodecID));
		}

		return Codec;
	}

	const AVCodec* FindDecodeCodec(const char* n_szName)
	{
		const AVCodec* Codec = avcodec_find_decoder_by_name(n_szName);
		ThrowExceptionExpr(!Codec, "Fail to find decoder: %s", n_szName);

		return Codec;
	}

	const AVCodec* FindEncodeCodec(AVCodecID n_CodecID,
		std::shared_ptr<FSetting> n_Setting)
	{
		const AVCodec* Codec = nullptr;

		if (n_Setting && n_Setting->bEnableHwAccel)
		{
			auto vHwCodecs = FindAllHwCodecs();
			for (size_t i = 0; i < vHwCodecs.size(); i++)
			{
				if (vHwCodecs[i].CodecType == ECodecType::CT_Encoder &&
					vHwCodecs[i].GraphicCard == n_Setting->GetGraphicCard() &&
					vHwCodecs[i].Codec->id == n_CodecID)
				{
					Codec = vHwCodecs[i].Codec;
					break;
				}
			}
		}

		if (!Codec) Codec = avcodec_find_encoder(n_CodecID);
		if (!Codec)
		{
			AVDebug("Fail to find encoder: %s",
				avcodec_get_name(n_CodecID));
		}

		return Codec;
	}

	const AVCodec* FindEncodeCodec(const char* n_szName)
	{
		const AVCodec* Codec = avcodec_find_encoder_by_name(n_szName);
		ThrowExceptionExpr(!Codec, "Fail to find encoder: %s", n_szName);

		return Codec;
	}

	const std::vector<std::string> GetHwDeviceTypes()
	{
		std::vector<std::string> vResult;

		auto type = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;

		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
		{
			vResult.emplace_back(av_hwdevice_get_type_name(type));
		}

		return vResult;
	}

	const std::vector<FHwCodec> FindAllHwCodecs()
	{
		static std::vector<FHwCodec> vResult;
		if (vResult.size() > 0) return vResult;

		void* nIndex = 0;
		const AVCodec* pCodec = nullptr;

		while (true)
		{
			pCodec = av_codec_iterate(&nIndex);
			if (!pCodec) break;

			if (pCodec && (pCodec->capabilities & AV_CODEC_CAP_HARDWARE))
			{
				FHwCodec HwCodec;
				HwCodec.Codec = pCodec;
				if (av_codec_is_decoder(pCodec))
					HwCodec.CodecType = ECodecType::CT_Decoder;
				else if (av_codec_is_encoder(pCodec))
					HwCodec.CodecType = ECodecType::CT_Encoder;
				HwCodec.Name = pCodec->name;

				if (strstr(pCodec->name, "_nvenc") ||
					strstr(pCodec->name, "_cuvid"))
					HwCodec.GraphicCard = EGraphicCard::GC_Nvidia;
				else if (strstr(pCodec->name, "_amf"))
					HwCodec.GraphicCard = EGraphicCard::GC_Amd;
				else if (strstr(pCodec->name, "_qsv"))
					HwCodec.GraphicCard = EGraphicCard::GC_Intel;

				vResult.emplace_back(HwCodec);
			}
		}

		return vResult;
	}

	int CompareCodecFormat(
		std::shared_ptr<FCodecContext> n_InputCodecContext,
		std::shared_ptr<FCodecContext> n_OutputCodecContext)
	{
		int ret = 0;

		if (!n_InputCodecContext || !n_OutputCodecContext)
			return ret;

		AVCodecContext* Input = n_InputCodecContext->Context;
		AVCodecContext* Output = n_OutputCodecContext->Context;

		if (Input->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			if (Input->codec_id != Output->codec_id ||
				Input->width != Output->width ||
				Input->height != Output->height ||
				n_InputCodecContext->GetPixelFormat() != 
				n_OutputCodecContext->GetPixelFormat())
				ret = 1;
		}
		else if (Input->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			if (Input->codec_id != Output->codec_id ||
				Input->sample_fmt != Output->sample_fmt ||
				Input->sample_rate != Output->sample_rate ||
				Input->ch_layout.nb_channels != Output->ch_layout.nb_channels)
				ret = 1;
		}

		return ret;
	}

	void CodecContextAddition(AVCodecContext* n_CodecContext)
	{
		/* Some formats want stream headers to be separate. */
		/* Some container formats (like MP4) require global headers to be present.
		 * Mark the encoder so that it behaves accordingly. */

		 //if (m_Context->oformat->flags & AVFMT_GLOBALHEADER)
		 //{
		 //	n_CodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		 //	unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67 };
		 //	n_CodecContext->extradata_size = 23;
		 //	n_CodecContext->extradata = (uint8_t*)av_malloc(23 + AV_INPUT_BUFFER_PADDING_SIZE);
		 //	memcpy_s(n_CodecContext->extradata, 23, sps_pps, 23);
		 //}

		if (n_CodecContext->codec_id == AVCodecID::AV_CODEC_ID_AAC)
		{
			if (n_CodecContext->bit_rate < 100 || n_CodecContext->bit_rate > 500000)
				n_CodecContext->bit_rate = 128000;
		}
		else if (n_CodecContext->codec_id == AVCodecID::AV_CODEC_ID_MPEG4)
		{
			if (n_CodecContext->time_base.den > 65535)
				n_CodecContext->time_base.den = 32768;
		}
	}

	int GetPixFmtPlaneCount(AVPixelFormat n_ePixelFormat)
	{
		return av_pix_fmt_count_planes(n_ePixelFormat);
	}

	const AVPixFmtDescriptor* GetPixFmtDesc(AVPixelFormat n_ePixelFormat)
	{
		return av_pix_fmt_desc_get(n_ePixelFormat);
	}

	int GetBitsPerPixel(const AVPixFmtDescriptor* n_PixDesc)
	{
		return av_get_bits_per_pixel(n_PixDesc);
	}

	int IsSampleFmtPlanar(AVSampleFormat n_eSampleFormat)
	{
		return av_sample_fmt_is_planar(n_eSampleFormat);
	}

	int GetBytesPerSample(AVSampleFormat n_eSampleFormat)
	{
		return av_get_bytes_per_sample(n_eSampleFormat);
	}

	AVCodecContext* CopyCodecContext(
		const AVCodecContext* n_CodecContext)
	{
		AVCodecContext* CodecContext = avcodec_alloc_context3(n_CodecContext->codec);
		if (!CodecContext) return nullptr;

		AVCodecParameters* par = avcodec_parameters_alloc();
		avcodec_parameters_from_context(par, n_CodecContext);

		avcodec_parameters_to_context(CodecContext, par);
		avcodec_parameters_free(&par);

		CodecContext->time_base = n_CodecContext->time_base;
		CodecContext->framerate = n_CodecContext->framerate;

		return CodecContext;
	}

}

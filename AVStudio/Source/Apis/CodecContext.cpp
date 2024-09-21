#include "Apis/CodecContext.h"
#include "Util/Debug.h"


namespace avstudio
{
	static AVPixelFormat GetHwFormat(AVCodecContext* n_CodecContext,
		const AVPixelFormat* n_PixelFormats)
	{
		const AVPixelFormat* p = n_PixelFormats;
		auto CodecContext = (FCodecContext*)n_CodecContext->opaque;
		auto ePixelFormat = CodecContext->GetHwPixelFormat();

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

	const AVCodec* FCodecContext::FindDecodeCodec(AVCodecID n_CodecID)
	{
		const AVCodec* Codec = nullptr;

		if (GetSetting()->bEnableHwAccel)
		{
			auto vHwCodecs = FindAllHwCodecs();
			for (size_t i = 0; i < vHwCodecs.size(); i++)
			{
				if (vHwCodecs[i].IsDecoder &&
					vHwCodecs[i].GraphicCard == GetSetting()->GetGraphicCard() &&
					vHwCodecs[i].Id == n_CodecID)
				{
					Codec = FindDecodeCodec(vHwCodecs[i].Name);
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

	const AVCodec* FCodecContext::FindDecodeCodec(const char* n_szName)
	{
		const AVCodec* Codec = avcodec_find_decoder_by_name(n_szName);
		ThrowExceptionExpr(!Codec, "Fail to find decoder: %s", n_szName);

		return Codec;
	}

	const AVCodec* FCodecContext::FindEncodeCodec(AVCodecID n_CodecID)
	{
		const AVCodec* Codec = nullptr;

		if (GetSetting()->bEnableHwAccel)
		{
			auto vHwCodecs = FindAllHwCodecs();
			for (size_t i = 0; i < vHwCodecs.size(); i++)
			{
				if (!vHwCodecs[i].IsDecoder &&
					vHwCodecs[i].GraphicCard == GetSetting()->GetGraphicCard() &&
					vHwCodecs[i].Id == n_CodecID)
				{
					Codec = FindEncodeCodec(vHwCodecs[i].Name);
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

	const AVCodec* FCodecContext::FindEncodeCodec(const char* n_szName)
	{
		const AVCodec* Codec = avcodec_find_encoder_by_name(n_szName);
		ThrowExceptionExpr(!Codec, "Fail to find encoder: %s", n_szName);

		return Codec;
	}

	const std::vector<std::string> FCodecContext::GetHwDeviceTypes()
	{
		std::vector<std::string> vResult;

		auto type = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;

		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
		{
			vResult.emplace_back(av_hwdevice_get_type_name(type));
		}

		return vResult;
	}

	const std::vector<FHwCodec> FCodecContext::FindAllHwCodecs()
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
				HwCodec.IsDecoder = av_codec_is_decoder(pCodec);
				HwCodec.Id = pCodec->id;
				HwCodec.Name = pCodec->name;

				if (strstr(pCodec->name, "_nvenc") ||
					strstr(pCodec->name, "_cuvid"))
					HwCodec.GraphicCard = kGraphicCardNvidia;
				else if (strstr(pCodec->name, "_amf"))
					HwCodec.GraphicCard = kGraphicCardAmd;
				else if (strstr(pCodec->name, "_qsv"))
					HwCodec.GraphicCard = kGraphicCardIntel;

				vResult.emplace_back(HwCodec);
			}
		}

		return vResult;
	}

	AVCodecContext* FCodecContext::Alloc(const AVCodec* n_Codec)
	{
		ThrowExceptionExpr(!n_Codec, 
			"Invalid parameter: n_Codec is nullptr.\n");

		Context = avcodec_alloc_context3(n_Codec);
		ThrowExceptionExpr(!Context, 
			"Fail to alloc codec context: %s", n_Codec->name);

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
		if (m_SwFrame) av_frame_free(&m_SwFrame);
	}

	int FCodecContext::GetHwPixelFormat(const AVCodec* n_Codec, 
		AVHWDeviceType n_eHwDeviceType, AVPixelFormat& n_ePixelFormat)
	{
		int ret = -1;
		n_ePixelFormat = AVPixelFormat::AV_PIX_FMT_NONE;

		if (n_Codec->type != AVMediaType::AVMEDIA_TYPE_VIDEO)
			return ret;

		for (int i = 0;;i++)
		{
			const AVCodecHWConfig* config = avcodec_get_hw_config(n_Codec, i);
			if (!config) break;

			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
				config->device_type == n_eHwDeviceType)
			{
				n_ePixelFormat = config->pix_fmt;
				ret = 0;
				break;
			}
		}

		return ret;
	}

	void FCodecContext::InitHardwareContext(AVHWDeviceType n_eHwDeviceType)
	{
		int ret = av_hwdevice_ctx_create(&m_HwDeviceContext, n_eHwDeviceType, 
			nullptr, nullptr, 0);

		ThrowExceptionCodeExpr(ret < 0, ret, "Failed to create specified HW device.\n");
		
		Context->hw_device_ctx = av_buffer_ref(m_HwDeviceContext);
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

	void FCodecContext::CopyCodecParameter(const AVCodecContext* n_CodecContext)
	{
		Alloc(n_CodecContext->codec);

		AVCodecParameters* par = avcodec_parameters_alloc();
		avcodec_parameters_from_context(par, n_CodecContext);

		avcodec_parameters_to_context(Context, par);
		avcodec_parameters_free(&par);

		Context->time_base = n_CodecContext->time_base;
		Context->framerate = n_CodecContext->framerate;
	}

	void FCodecContext::Open(AVDictionary** n_Options)
	{
		ThrowExceptionExpr(!Context, "You should call function Alloc() first.\n");

		if (avcodec_is_open(Context)) return;

		if (GetSetting()->bEnableHwAccel &&
			Context->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			auto Codec = Context->codec;
			if (Codec->capabilities & AV_CODEC_CAP_HARDWARE)
			{
				auto ret = GetHwPixelFormat(Context->codec, 
					GetSetting()->GetHwDeviceType(), m_eHwPixelFormat);

				if (ret >= 0)
				{
					InitHardwareContext(GetSetting()->GetHwDeviceType());
					Context->thread_count = 0;
					Context->get_format = GetHwFormat;
					Context->opaque = this;
				}
			}
		}

		int ret = avcodec_open2(Context, Context->codec, n_Options);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to open codec context.");
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
			if (m_Frame && m_Frame->format == m_eHwPixelFormat)
			{
				if (!m_SwFrame) m_SwFrame = av_frame_alloc();
				/* Retrieve data from GPU to CPU */
				ret = av_hwframe_transfer_data(m_SwFrame, m_Frame, 0);
				ThrowExceptionExpr(ret < 0 || !m_SwFrame, 
					"Error during decoding: %s\n",
					ErrorCode2String(ret).c_str());

				m_SwFrame->pts = m_Frame->pts;
				m_SwFrame->pkt_dts = m_Frame->pkt_dts;
				m_SwFrame->best_effort_timestamp = m_Frame->best_effort_timestamp;
				m_eGraphicCardPixelFormat = (AVPixelFormat)m_SwFrame->format;

				Frame = m_SwFrame;
			}

			if (n_Func) n_Func(Frame);
			if (Frame == m_SwFrame) av_frame_unref(Frame);
		}

		return ret;
	}

	int FCodecContext::EncodeFrame(const AVFrame* n_Frame, 
		std::function<int(AVPacket* n_Packet)> n_Func)
	{
		ThrowExceptionExpr(!Context, 
			"You should call function Alloc() first.\n");

		int ret = avcodec_send_frame(Context, n_Frame);
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

	int FCodecContext::GetPixFmtPlaneCount()
	{
		if (Context->codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO)
			return av_pix_fmt_count_planes(Context->pix_fmt);

		return 0;
	}

	int FCodecContext::IsSampleFmtPlanar()
	{
		if (Context->codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO)
			return av_sample_fmt_is_planar(Context->sample_fmt);

		return 0;
	}

	int FCodecContext::GetBytesPerSample()
	{
		if (Context->codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO)
			return av_get_bytes_per_sample(Context->sample_fmt);

		return 0;
	}

	const AVPixelFormat FCodecContext::GetHwPixelFormat() const
	{
		return m_eHwPixelFormat;
	}

	const AVPixelFormat FCodecContext::GetPixelFormat() const
	{
		if (!Context) return AVPixelFormat::AV_PIX_FMT_NONE;
		if (m_eGraphicCardPixelFormat == AVPixelFormat::AV_PIX_FMT_NONE)
			return Context->pix_fmt;

		return m_eGraphicCardPixelFormat;
	}

	//////////////////////////////////////////////////////////////////////
	int CompareCodecFormat(FCodecContext* n_InputCodecContext,
		FCodecContext* n_OutputCodecContext)
	{
		int ret = 0;

		if (!n_InputCodecContext || !n_OutputCodecContext)
			return ret;

		AVCodecContext* Input = n_InputCodecContext->Context;
		AVCodecContext* Output = n_OutputCodecContext->Context;

		if (Input->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			auto fmt = Output->pix_fmt;

			if (Output->hw_frames_ctx)
			{
				auto frames_ctx = (AVHWFramesContext*)(Output->hw_frames_ctx->data);
				fmt = frames_ctx->sw_format;
			}

			if (Input->codec_id != Output->codec_id ||
				Input->width != Output->width ||
				Input->height != Output->height ||
				n_InputCodecContext->GetPixelFormat() != fmt)
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
				n_CodecContext->time_base.den = 65535;
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

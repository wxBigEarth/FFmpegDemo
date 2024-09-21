#include "Apis/FormatContext.h"
#include "Util/Debug.h"


namespace avstudio
{
	FFormatContext::~FFormatContext()
	{
		Release();
	}

	AVFormatContext* FFormatContext::OpenInputFile(const std::string& n_sFile, 
		const AVInputFormat* n_InputFormat, AVDictionary* n_Options)
	{
		int ret = avformat_open_input(&Context, n_sFile.c_str(), 
			n_InputFormat, &n_Options);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to open input file.");

		ret = avformat_find_stream_info(Context, nullptr);
		ThrowExceptionCodeExpr(ret < 0, ret, 
			"Fail to find stream info about the input file.");

		Name = n_sFile;

		return Context;
	}

	AVFormatContext* FFormatContext::AllocOutputFile(const std::string& n_sFile, 
		const AVOutputFormat* n_OutputFormat /*= nullptr*/, 
		const char* n_szFormatName /*= nullptr*/)
	{
		std::remove(n_sFile.c_str());

		int ret = avformat_alloc_output_context2(&Context, n_OutputFormat, 
			n_szFormatName, n_sFile.c_str());
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to alloc output file.");

		Name = n_sFile;

		return Context;
	}

	void FFormatContext::Release()
	{
		if (Context)
		{
			if (Context->iformat)
				avformat_close_input(&Context);
			else
			{
				if (Context->pb) avio_close(Context->pb);

				avformat_free_context(Context);
				Context = nullptr;
			}
		}
	}

	const unsigned int FFormatContext::StreamSize()
	{
		if (!Context) return 0;

		return Context->nb_streams;
	}

	const double FFormatContext::Length()
	{
		return Context ? Context->duration * 1.0 / AV_TIME_BASE : 0;
	}

	const bool FFormatContext::IsValid() const
	{
		return !Name.empty() && Context;
	}

	AVStream* FFormatContext::FindStream(unsigned int n_nStreamIndex)
	{
		if (!IsValid() || n_nStreamIndex >= StreamSize()) return nullptr;
		return Context->streams[n_nStreamIndex];
	}

	int FFormatContext::FindStreamIndex(AVMediaType n_eMediaType, const AVCodec** n_Codec)
	{
		if (!Context) return -1;

		return av_find_best_stream(Context, n_eMediaType, -1, -1, n_Codec, 0);
	}

	const AVMediaType FFormatContext::GetStreamMediaType(unsigned int n_nStreamIndex)
	{
		auto Stream = FindStream(n_nStreamIndex);
		if (!Stream) return AVMediaType::AVMEDIA_TYPE_UNKNOWN;

		return Stream->codecpar->codec_type;
	}

	AVStream* FFormatContext::BuildStream(AVCodecContext* n_CodecContext)
	{
		AVStream* Stream = avformat_new_stream(Context, nullptr);
		ThrowExceptionExpr(!Stream, "Fail to create stream\n");

		int ret = avcodec_parameters_from_context(Stream->codecpar, n_CodecContext);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to copy parameters from codec context.");

		Stream->r_frame_rate = n_CodecContext->framerate;
		Stream->codecpar->codec_tag = n_CodecContext->codec_tag;

		return Stream;
	}

	AVStream* FFormatContext::BuildStream(AVStream* n_Stream)
	{
		if (!n_Stream) return nullptr;

		AVStream* Stream = avformat_new_stream(Context, nullptr);
		ThrowExceptionExpr(!Stream, "Fail to create stream\n");

		int ret = avcodec_parameters_copy(Stream->codecpar, n_Stream->codecpar);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to copy parameters from stream.");

		Stream->codecpar->codec_tag = n_Stream->codecpar->codec_tag;

		return Stream;
	}

	int FFormatContext::ReadPacket(AVPacket* n_Packet) const
	{
		int ret = av_read_frame(Context, n_Packet);
		ThrowExceptionCodeExpr(ret < 0 && ret != AVERROR_EOF, ret, "Fail to read packet.");

		return ret;
	}

	void FFormatContext::OpenOutputFile()
	{
		if (!IsValid()) return;

		int ret = avio_open(&Context->pb, Context->url, AVIO_FLAG_WRITE);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to open file.");

		m_bWriteHeader = false;
	}

	void FFormatContext::WriteHeader()
	{
		if (!IsValid()) return;

		int ret = avformat_write_header(Context, nullptr);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to write header into output file.");

		m_bWriteHeader = true;
	}

	int FFormatContext::InterleavedWritePacket(AVPacket* n_Packet)
	{
#if _DEBUG
		RescalePacket(n_Packet);
#endif

		int ret = av_interleaved_write_frame(Context, n_Packet);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to write packet into output file.");

		return ret;
	}

	int FFormatContext::WritePacket(AVPacket* n_Packet)
	{
#if _DEBUG
		RescalePacket(n_Packet);
#endif

		int ret = av_write_frame(Context, n_Packet);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to write packet into output file.");

		return ret;
	}

	void FFormatContext::WriteTrailer()
	{
		if (!IsValid()) return;
		if (!m_bWriteHeader) return;
		int ret = av_write_trailer(Context);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to write trailer into output file.");
	}

	void FFormatContext::RescalePacket(AVPacket* n_Packet)
	{
		AVDebug("StreamIndex: %d; pts: %zd => dts: %zd\n", 
			n_Packet->stream_index, n_Packet->pts, n_Packet->dts);
	}

	//////////////////////////////////////////////////////////////////////////
	AVSampleFormat GetSupportedSampleFormat(const AVCodec* n_Codec, 
		enum AVSampleFormat n_eSampleFormat)
	{
		if (!n_Codec || !n_Codec->sample_fmts || 
			AVSampleFormat::AV_SAMPLE_FMT_NONE == *n_Codec->sample_fmts)
			return n_eSampleFormat;

		const enum AVSampleFormat* p = n_Codec->sample_fmts;

		AVSampleFormat SampleFormat = *p;

		while (p && *p != AVSampleFormat::AV_SAMPLE_FMT_NONE)
		{
			if (*p == n_eSampleFormat)
			{
				SampleFormat = *p;
				break;
			}

			p++;
		}

		return SampleFormat;
	}

	int GetSupportedSampleRate(const AVCodec* n_Codec, int n_nSampleRate)
	{
		if (!n_Codec || 
			!n_Codec->supported_samplerates || 
			0 == *n_Codec->supported_samplerates)
			return n_nSampleRate;

		const int* p = n_Codec->supported_samplerates;

		int nSampleRate = *p;
		int nDiff = abs(n_nSampleRate - *p);
		p++;

		while (p && *p)
		{
			int v = abs(n_nSampleRate - *p);
			if (nDiff > v)
			{
				nDiff = v;
				nSampleRate = *p;
			}

			p++;
		}

		return nSampleRate;
	}

	int GetSupportedChannelLayout(const AVCodec* n_Codec, 
		AVChannelLayout* n_ChannelLayout)
	{
		if (!n_Codec || !n_Codec->ch_layouts)
		{
			AVChannelLayout ChannelLayout;
			ChannelLayout.order = AV_CHANNEL_ORDER_NATIVE;
			ChannelLayout.nb_channels = 2;
			ChannelLayout.u.mask = AV_CH_LAYOUT_STEREO;

			return av_channel_layout_copy(n_ChannelLayout, &ChannelLayout);
		}

		const AVChannelLayout* p = n_Codec->ch_layouts;
		const AVChannelLayout* BestChannelLayout = p;
		int nBestNbChannels = 0;

		while (p && p->nb_channels)
		{
			if (p->nb_channels > nBestNbChannels)
			{
				nBestNbChannels = p->nb_channels;
				BestChannelLayout = p;
			}

			p++;
		}

		return av_channel_layout_copy(n_ChannelLayout, BestChannelLayout);
	}

	AVPixelFormat GetSupportedPixelFormat(const AVCodec* n_Codec, 
		AVPixelFormat n_ePixelFormat)
	{
		if (!n_Codec || !n_Codec->pix_fmts ||
			AVPixelFormat::AV_PIX_FMT_NONE == *n_Codec->pix_fmts)
			return n_ePixelFormat;

		const AVPixelFormat* p = n_Codec->pix_fmts;
		AVPixelFormat PixelFormat = *p;

		while (p && *p != AVPixelFormat::AV_PIX_FMT_NONE)
		{
			if (*p == n_ePixelFormat)
			{
				PixelFormat = *p;
				break;
			}

			p++;
		}

		return PixelFormat;
	}

	AVRational GetSupportedFrameRate(const AVCodec* n_Codec, 
		const AVRational& n_FrameRate)
	{
		if (!n_Codec || !n_Codec->supported_framerates)
			return n_FrameRate;

		const AVRational* p = n_Codec->supported_framerates;

		if (!p || p->num == 0 || p->den == 0) return n_FrameRate;

		AVRational Rational = *p++;
		double dBase = av_q2d(n_FrameRate);
		double dCurrent = av_q2d(Rational);
		double dDiff = abs(dCurrent - dBase);

		while (p && p->num > 0 && p->den > 0)
		{
			if (p->num)
			{
				dCurrent = av_q2d(*p);
				double v = abs(dCurrent - dBase);

				if (dDiff > v)
				{
					dDiff = v;
					Rational = *p;
				}
			}

			p++;
		}

		return Rational;
	}

	int FindStreamIndex(AVFormatContext* n_FormatContext, AVMediaType n_eMediaType)
	{
		return av_find_best_stream(n_FormatContext, n_eMediaType, -1, -1, nullptr, 0);
	}

	const AVInputFormat* FindInputFormat(const std::string& n_sShortName)
	{
		return av_find_input_format(n_sShortName.c_str());
	}
}

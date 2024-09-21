#include "Apis/Frame.h"
#include "Util/Debug.h"


namespace avstudio
{
	FFrame::~FFrame()
	{
		Release();
	}

	AVFrame* FFrame::Alloc()
	{
		Self = av_frame_alloc();
		ThrowExceptionExpr(!Self, "Fail to create frame.\n");

		Self->pts = 0;
		Self->pkt_dts = 0;
		Self->duration = 0;

		return Self;
	}

	void FFrame::Release()
	{
		if (Self)
		{
			av_frame_free(&Self);
			Self = nullptr;
		}
	}

	AVFrame* FFrame::Clone(const AVFrame* n_Frame)
	{
		Self = av_frame_clone(n_Frame);
		return Self;
	}

	void FFrame::MoveRef(AVFrame* n_Frame)
	{
		if (!Self) Alloc();
		av_frame_move_ref(Self, n_Frame);
	}

	void FFrame::UnRef()
	{
		if (Self) av_frame_unref(Self);
	}

	void FFrame::MakeWritable()
	{
		int ret = av_frame_make_writable(Self);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to make frame writable.");
	}

	void FFrame::AllocVideoBuffer(const int n_nWidth, const int n_nHeight, 
		const AVPixelFormat n_ePixelFormat)
	{
		if (!Self) Alloc();

		Self->width = n_nWidth;
		Self->height = n_nHeight;
		Self->format = n_ePixelFormat;

		int ret = av_frame_get_buffer(Self, 0);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to alloc buffer for video frame.");
	}

	void FFrame::AllocVideoBuffer(const AVCodecContext* n_CodecContext)
	{
		AllocVideoBuffer(n_CodecContext->width, n_CodecContext->height, 
			n_CodecContext->pix_fmt);
	}

	void FFrame::AllocAudioBuffer(const int n_nSamples, const int n_nSampleRate, 
		const AVSampleFormat n_eSampleFormat, const AVChannelLayout* n_ChannelLayout)
	{
		if (!Self) Alloc();

		Self->nb_samples = n_nSamples;
		Self->sample_rate = n_nSampleRate;
		Self->format = n_eSampleFormat;

		int ret = av_channel_layout_copy(&Self->ch_layout, n_ChannelLayout);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to copy channel layout for audio frame.");

		ret = av_frame_get_buffer(Self, 0);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to alloc buffer for audio frame.");
	}

	void FFrame::AllocAudioBuffer(const AVCodecContext* n_CodecContext)
	{
		AllocAudioBuffer(n_CodecContext->frame_size, n_CodecContext->sample_rate, 
			n_CodecContext->sample_fmt, &n_CodecContext->ch_layout);
	}

	AVFrame* FFrame::VideoFrame(const int n_nWidth, const int n_nHeight, 
		const AVPixelFormat n_ePixelFormat, AVFrame* n_Frame)
	{
		AVFrame* Frame = n_Frame;
		if (!Frame) Frame =	av_frame_alloc();
		ThrowExceptionExpr(!Frame, "Fail to create frame.\n");

		Frame->pts = 0;
		Frame->pkt_dts = 0;
		Frame->duration = 0;

		Frame->width = n_nWidth;
		Frame->height = n_nHeight;
		Frame->format = n_ePixelFormat;

		int ret = av_frame_get_buffer(Frame, 0);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to alloc buffer for video frame.");

		ret = av_frame_make_writable(Frame);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to make frame writable.");

		return Frame;
	}

	AVFrame* FFrame::AudioFrame(const int n_nSamples, const int n_nSampleRate, 
		const AVSampleFormat n_eSampleFormat, 
		const AVChannelLayout* n_ChannelLayout,
		AVFrame* n_Frame)
	{
		AVFrame* Frame = n_Frame;
		if (!Frame) Frame = av_frame_alloc();
		ThrowExceptionExpr(!Frame, "Fail to create frame.\n");

		Frame->pts = 0;
		Frame->pkt_dts = 0;
		Frame->duration = n_nSamples;

		Frame->nb_samples = n_nSamples;
		Frame->sample_rate = n_nSampleRate;
		Frame->format = n_eSampleFormat;

		int ret = av_channel_layout_copy(&Frame->ch_layout, n_ChannelLayout);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to copy channel layout for audio frame.");

		ret = av_frame_get_buffer(Frame, 0);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to alloc buffer for audio frame.");

		ret = av_frame_make_writable(Frame);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to make frame writable.");

		return Frame;
	}

}
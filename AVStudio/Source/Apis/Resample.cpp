#include "Apis/Resample.h"
#include "Util/Debug.h"


namespace avstudio
{
	FResample::~FResample()
	{
		Release();
	}

	SwrContext* FResample::Alloc(
		const AVChannelLayout* n_InputChannelLayout, 
		const AVSampleFormat& n_eInputSampleFormat, 
		const int& n_nInputSampleRate, 
		const AVChannelLayout* n_OutputChannelLayout, 
		const AVSampleFormat& n_eOutputSampleFormat, 
		const int& n_nOutputSampleRate)
	{
		Release();

		int ret = swr_alloc_set_opts2(&Context,
			n_OutputChannelLayout, n_eOutputSampleFormat, n_nOutputSampleRate,
			n_InputChannelLayout, n_eInputSampleFormat, n_nInputSampleRate,
			0, nullptr);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to alloc swr context.");

		ret = swr_init(Context);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to init swr context.");

		m_InputChannels = n_InputChannelLayout->nb_channels;
		m_nInputSampleRate = n_nInputSampleRate;
		m_eInputSampleFormat = n_eInputSampleFormat;
		m_OutputChannels = n_OutputChannelLayout->nb_channels;
		m_nOutputSampleRate = n_nOutputSampleRate;
		m_eOutputSampleFormat = n_eOutputSampleFormat;

		return Context;
	}

	SwrContext* FResample::Alloc(AVCodecContext* n_InputCodecContext, 
		AVCodecContext* n_OutputCodecContext)
	{
		return Alloc(
			&n_InputCodecContext->ch_layout, 
			n_InputCodecContext->sample_fmt, 
			n_InputCodecContext->sample_rate,
			&n_OutputCodecContext->ch_layout, 
			n_OutputCodecContext->sample_fmt, 
			n_OutputCodecContext->sample_rate);
	}

	void FResample::Release()
	{
		nSamples = 0;
		if (CoverData)
		{
			av_freep(&CoverData[0]);
			free(CoverData);
			CoverData = nullptr;
		}

		if (Context) swr_free(&Context);
	}

	int FResample::Cover(const uint8_t** n_InputData, int n_nInputFrameSize)
	{
		if (!CoverData)
		{
			AllocCovertedSample(m_OutputChannels, n_nInputFrameSize,
				m_eOutputSampleFormat);
		}

		return Cover(n_InputData, n_nInputFrameSize, CoverData, nSamples);
	}

	int FResample::Cover(const uint8_t** n_InputData, int n_nInputFrameSize, 
		uint8_t** n_OutputData, int n_nOutputFrameSize)
	{
		ThrowExceptionExpr(!Context,
			"You should call function Alloc first.\n");

		int ret = swr_convert(Context, n_OutputData, n_nOutputFrameSize,
			n_InputData, n_nInputFrameSize);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to cover audio data.");

		return ret;
	}

	int FResample::Cover(const AVFrame* n_InputFrame)
	{
		const uint8_t** pData = (const uint8_t**)n_InputFrame->extended_data;
		return Cover(pData, n_InputFrame->nb_samples);
	}

	void FResample::AllocCovertedSample(const int& n_nChannels, const int& n_nFrameSize,
		const AVSampleFormat& n_eOutputSampleFormat)
	{
		if (CoverData)
		{
			av_freep(&CoverData[0]);
			free(CoverData);
		}

		nSamples = (int)av_rescale_rnd(n_nFrameSize, m_nOutputSampleRate, m_nInputSampleRate, AV_ROUND_UP);

		/* Allocate as many pointers as there are audio channels.
		 * Each pointer will later point to the audio samples of the corresponding
		 * channels (although it may be NULL for interleaved formats).
		 */
		CoverData = (uint8_t**)calloc(n_nChannels, sizeof(*CoverData));
		ThrowExceptionExpr(!CoverData,
			"Fail to allocate converted input sample pointers.");

		/* Allocate memory for the samples of all channels in one consecutive
		 * block for convenience. */
		int ret = av_samples_alloc(CoverData, nullptr, n_nChannels,
			nSamples, n_eOutputSampleFormat, 0);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to allocate converted input samples.");
	}
}
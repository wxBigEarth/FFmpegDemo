#ifndef __RESAMPLE_H__
#define __RESAMPLE_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	struct FResample
	{
		FResample() = default;
		~FResample();

		// Alloc Swr context
		SwrContext* Alloc(
			const AVChannelLayout* n_InputChannelLayout,
			const AVSampleFormat& n_eInputSampleFormat,
			const int& n_nInputSampleRate,
			const AVChannelLayout* n_OutputChannelLayout,
			const AVSampleFormat& n_eOutputSampleFormat,
			const int& n_nOutputSampleRate);

		SwrContext* Alloc(AVCodecContext* n_InputCodecContext,
			AVCodecContext* n_OutputCodecContext);

		// Release 
		void Release();

		// Cover audio data
		// return number of samples output per channel, negative value on error
		int Cover(const uint8_t** n_InputData, int n_nInputFrameSize);
		int Cover(const uint8_t** n_InputData, int n_nInputFrameSize, 
			uint8_t** n_OutputData, int n_nOutputFrameSize);
		int Cover(const AVFrame* n_InputFrame);

		void AllocCovertedSample(const int& n_nChannels, const int& n_nFrameSize,
			const AVSampleFormat& n_eOutputSampleFormat);

		// Get parameters
		int					GetInputChannels()		const { return m_InputChannels; }
		int					GetInputSampleRate()	const { return m_nInputSampleRate; }
		AVSampleFormat		GetInputSampleFormat()	const { return m_eInputSampleFormat; }
		int					GetOutputChannels()		const { return m_OutputChannels; }
		int					GetOutputSampleRate()	const { return m_nOutputSampleRate; }
		AVSampleFormat		GetOutputSampleFormat()	const { return m_eOutputSampleFormat; }

		SwrContext*			Context = nullptr;

		// Output data when converter audio frame data
		uint8_t**			CoverData = nullptr;
		int					nSamples = 0;

	protected:
		int					m_InputChannels			= 0;
		int					m_nInputSampleRate		= 0;
		AVSampleFormat		m_eInputSampleFormat	= AVSampleFormat::AV_SAMPLE_FMT_NONE;
		int					m_OutputChannels		= 0;
		int					m_nOutputSampleRate		= 0;
		AVSampleFormat		m_eOutputSampleFormat	= AVSampleFormat::AV_SAMPLE_FMT_NONE;
	};
}

#endif // !__RESAMPLE_H__
#ifndef __AUDIOFIFO_H__
#define __AUDIOFIFO_H__
#include <memory>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/audio_fifo.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	struct FAudioFifo
	{
		FAudioFifo() = default;
		~FAudioFifo();

		/*
		* Alloc audio FIFO buffer
		* parameters:
		*	const AVSampleFormat& n_eSampleFormat:
		*		Sample format of the output frame
		*	const int& n_nChannels:
		*		Number of channels of output frame
		*	const int& n_nSamples:
		*		Number of samples per channel of output frame
		*/
		AVAudioFifo* Alloc(const AVSampleFormat& n_eSampleFormat,
			const int& n_nChannels, const int& n_nSamples);

		void Release();

		int Size() const;
		int OutputFrameSize();
		// The number of samples for next frame
		int NextSampleCount();

		// Push data into fifo buffer
		void Push(uint8_t** n_Samples, const int& n_nSize);

		// Pop frame out from fifo buffer
		void Pop(uint8_t** n_Samples, const int& n_nSize);
		void Pop(AVFrame* n_Frame);

		// Could read frame or not
		bool IsReadable() const;

		// Get parameters
		int				GetChannels()		const { return m_nChannels; }
		int				GetSamples()		const { return m_nSamples; }
		AVSampleFormat	GetSampleFormat()	const { return m_eSampleFormat; }

		AVAudioFifo*	Context = nullptr;

	protected:
		// Sample format of the output frame
		AVSampleFormat	m_eSampleFormat = AVSampleFormat::AV_SAMPLE_FMT_NONE;
		// Number of channels of output frame
		int				m_nChannels = 0;
		// Number of samples per channel of output frame
		int				m_nSamples = 0;
	};

	void MoveAudioFifoData(
		std::shared_ptr<FAudioFifo> n_Src, std::shared_ptr<FAudioFifo> n_Des);
}
#endif // !__AUDIOFIFO_H__

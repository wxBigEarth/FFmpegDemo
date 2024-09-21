#include "Apis/AudioFifo.h"
#include "Util/Debug.h"


namespace avstudio
{
	FAudioFifo::~FAudioFifo()
	{
		Release();
	}

	AVAudioFifo* FAudioFifo::Alloc(const AVSampleFormat& n_eSampleFormat,
		const int& n_nChannels, const int& n_nSamples)
	{
		m_eSampleFormat = n_eSampleFormat;
		m_nChannels = n_nChannels;
		m_nSamples = n_nSamples;

		Context = av_audio_fifo_alloc(m_eSampleFormat, m_nChannels, 1);

		ThrowExceptionExpr(!Context,
			"Fail to create audio FIFO buffer.\n");

		return Context;
	}

	void FAudioFifo::Release()
	{
		if (Context)
		{
			av_audio_fifo_free(Context);
			Context = nullptr;
		}
	}

	int FAudioFifo::Size() const
	{
		return Context ? av_audio_fifo_size(Context) : 0;
	}

	int FAudioFifo::OutputFrameSize()
	{
		return m_nSamples;
	}

	int FAudioFifo::NextSampleCount()
	{
		int nSize = Size();
		if (nSize > m_nSamples)
			nSize = m_nSamples;

		return nSize;
	}

	void FAudioFifo::Push(uint8_t** n_Samples, const int& n_nSize)
	{
		if (!Context) return;

		int ret = 0;
		int nSize = Size();

		if (av_audio_fifo_space(Context) < n_nSize)
		{
			ret = av_audio_fifo_realloc(Context, nSize + n_nSize);
			ThrowExceptionCodeExpr(ret < 0, ret, "Fail to realloc FIFO buffer.\n");
		}

		if (n_nSize == 0) return;

		ret = av_audio_fifo_write(Context, (void**)n_Samples, n_nSize);
		ThrowExceptionCodeExpr(ret < n_nSize, ret, 
			"Fail to write audio samples into FIFO buffer.\n");
	}

	void FAudioFifo::Pop(uint8_t** n_Samples, const int& n_nSize)
	{
		if (!Context) return;

		int ret = av_audio_fifo_read(Context, (void**)n_Samples, n_nSize);
		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to read from fifo buffer.\n");
	}

	void FAudioFifo::Pop(AVFrame* n_Frame)
	{
		Pop(n_Frame->data, n_Frame->nb_samples);
	}

	bool FAudioFifo::IsReadable()
	{
		int nSize = Size();

		return nSize > 0 && m_nSamples <= nSize;
	}

	void MoveAudioFifoData(FAudioFifo* n_Src, FAudioFifo* n_Des)
	{
		if (!n_Src || !n_Des) return;

		if (n_Src->GetChannels() != n_Des->GetChannels() ||
			n_Src->GetSampleFormat() != n_Des->GetSampleFormat() ||
			n_Src->GetSamples() != n_Des->GetSamples())
			return;

		uint8_t** Buffer = nullptr;
		/* Allocate as many pointers as there are audio channels.
		 * Each pointer will later point to the audio samples of the corresponding
		 * channels (although it may be NULL for interleaved formats).
		 */
		Buffer = (uint8_t**)calloc(n_Src->GetChannels(), sizeof(*Buffer));
		ThrowExceptionExpr(!Buffer,
			"Fail to allocate converted input sample pointers.");

		/* Allocate memory for the samples of all channels in one consecutive
		 * block for convenience. */
		int ret = av_samples_alloc(
			Buffer, 
			nullptr, 
			n_Src->GetChannels(),
			n_Src->GetSamples(), 
			n_Src->GetSampleFormat(),
			0);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to allocate converted input samples.");

		while (n_Src->IsReadable())
		{
			n_Src->Pop(Buffer, n_Src->GetSamples());
			n_Des->Push(Buffer, n_Src->GetSamples());
		}

		auto nSize = n_Src->Size();
		if (nSize > 0)
		{
			n_Src->Pop(Buffer, nSize);
			n_Des->Push(Buffer, nSize);
		}

		av_freep(&Buffer[0]);
		free(Buffer);
		Buffer = nullptr;
	}

}

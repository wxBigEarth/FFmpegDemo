#ifndef __EDITCONTEXT_H__
#define __EDITCONTEXT_H__
#include "IO/IOHandle.h"
#include "Core/WorkShop.h"


namespace avstudio
{
	class CFactory
	{
	public:
		CFactory() = default;
		CFactory(std::shared_ptr<FWorkShop> n_Output);
		~CFactory();

		std::shared_ptr<FWorkShop> Input();

		/*
		* For input context, Before start, do something
		*	const bool n_bIsLastItem: is it the last item in current group
		*	const bool n_bIsLastGroup: is it the last group
		*/
		int Processing(const bool n_bIsLastItem, const bool n_bIsLastGroup);
		// For input context, Do something while ending
		int WindUp();

		// Start Work
		int Editing();

		// Usually for writing PCM data
		//	AVFrame* n_Frame: Null indecates end
		int WriteFrame(AVFrame* n_Frame, AVMediaType n_eMediaType);

		// Flush FFmpeg buffer queue while ending
		void Flush();

		void SetIoHandle(std::shared_ptr<IIOHandle> n_Handle);

		const bool IsEnd() const;

		void Release();

	protected:
		int Demuxing(AVPacket* n_Packet);
		int Decoding(AVPacket* n_Packet, AVMediaType n_eMediaType);
		int Converting(AVFrame* n_Frame, AVMediaType n_eMediaType);
		int Filtering(AVFrame* n_Frame, AVMediaType n_eMediaType);
		int Encoding(AVFrame* n_Frame, AVMediaType n_eMediaType);

		// Push AVPacket/AVFrame into buffer
		void PushData(AVMediaType n_eMediaType, 
			EDataType n_eDataType, void* n_Data);

		// Pop frame from AVAudioFifo buffer
		int PopFromFifo();

		// Create audio AVframe with output codec
		AVFrame* AllocAudioFrame(int n_nFrameSize);

	protected:
		std::shared_ptr<FWorkShop>	m_Input = nullptr;
		// Point to output context
		std::shared_ptr<FWorkShop>	m_Output = nullptr;
		// Point to IO handle.
		std::shared_ptr<IIOHandle>	m_IoHandle = nullptr;

		// For Demuxing
		AVPacket*	m_Packet = nullptr;
		// Video frame, for converting
		AVFrame*	m_vFrame = nullptr;
		// Audio frame, for converting
		AVFrame*	m_aFrame = nullptr;

		// Indicate if it's end
		bool		m_bIsEnd = false;
		// Indicate if it's last input context in current group
		bool		m_bIsLastItem = false;
		// Indicate if current group is the last one
		bool		m_bIsLastGroup = false;

		// If codec id is mp3 for audio stream, maybe nb_samples of first frame
		// is less than 1152, discard it. But the last frame could not be discarded
		bool		m_bFirstMp3Frame = true;
	};
}

#endif // !__EDITCONTEXT_H__
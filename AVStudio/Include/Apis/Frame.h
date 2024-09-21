#ifndef __FRAME_H__
#define __FRAME_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	struct FFrame
	{
		FFrame() = default;
		~FFrame();

		// Alloc packet memory
		AVFrame* Alloc();
		// Release 
		void Release();

		// Copy frame. No need to alloc m_Frame
		AVFrame* Clone(const AVFrame* n_Frame);
		// Move frame ref
		void MoveRef(AVFrame* n_Frame);
		// Unref frame
		void UnRef();
		// Make writable
		void MakeWritable();

		// Alloc data buffer for video frame
		void AllocVideoBuffer(const int n_nWidth, const int n_nHeight, 
			const AVPixelFormat n_ePixelFormat);
		void AllocVideoBuffer(const AVCodecContext* n_CodecContext);
		// Alloc data buffer for audio frame
		void AllocAudioBuffer(const int n_nSamples, const int n_nSampleRate, 
			const AVSampleFormat n_eSampleFormat, const AVChannelLayout* n_ChannelLayout);
		void AllocAudioBuffer(const AVCodecContext* n_CodecContext);

		// Alloc frame for video, alloc data buffer for writing
		static AVFrame* VideoFrame(const int n_nWidth, const int n_nHeight, 
			const AVPixelFormat n_ePixelFormat, AVFrame* n_Frame = nullptr);
		// Alloc frame for audio, alloc data buffer for writing
		static AVFrame* AudioFrame(const int n_nSamples, const int n_nSampleRate,
			const AVSampleFormat n_eSampleFormat, 
			const AVChannelLayout* n_ChannelLayout,
			AVFrame* n_Frame = nullptr);

		AVFrame* Self = nullptr;
	};
}

#endif // !__FRAME_H__

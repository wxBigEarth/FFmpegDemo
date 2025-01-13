#ifndef __CALE_H__
#define __CALE_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	struct FSwsScale
	{
		FSwsScale() = default;
		~FSwsScale();

		// Alloc sws context
		SwsContext* Alloc(
			const int& n_nInputWidth,
			const int& n_nInputHeight,
			const AVPixelFormat& n_eInputPixelFormat,
			const int& n_nOutputWidth,
			const int& n_nOutputHeight,
			const AVPixelFormat& n_eOutputPixelFormat);

		SwsContext* Alloc(AVCodecContext* n_InputCodecContext,
			AVCodecContext* n_OutputCodecContext);

		// Release 
		void Release();

		// Scale video data
		// return          the height of the output slice
		int Scale(uint8_t** n_InputData, const int* n_InputLineSize);
		int Scale(uint8_t** n_InputData, const int* n_InputLineSize,
			uint8_t** n_OutputData, const int* n_OuputLineSize);
		int Scale(const AVFrame* n_InputFrame);

		// Get parameters
		int				GetInputWidth()			const { return m_nInputWidth; }
		int				GetInputHeight()		const { return m_nInputHeight; }
		AVPixelFormat	GetInputPixelFormat()	const { return m_eInputPixelFormat; }
		int				GetOutputWidth()		const { return m_nOutputWidth; }
		int				GetOutputHeight()		const { return m_nOutputHeight; }
		AVPixelFormat	GetOutputPixelFormat()	const { return m_eOutputPixelFormat; }

		SwsContext*		Context = nullptr;

		// Output data when scale video frame data
		uint8_t*		CaleData[4] = {nullptr};
		int				LineSize[4] = { 0 };

	protected:
		int				m_nInputWidth			= 0;
		int				m_nInputHeight			= 0;
		AVPixelFormat	m_eInputPixelFormat		= AVPixelFormat::AV_PIX_FMT_NONE;
		int				m_nOutputWidth			= 0;
		int				m_nOutputHeight			= 0;
		AVPixelFormat	m_eOutputPixelFormat	= AVPixelFormat::AV_PIX_FMT_NONE;
	};
}

#endif // __CALE_H__


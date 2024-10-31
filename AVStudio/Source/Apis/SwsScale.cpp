#include "Apis/SwsScale.h"
#include "Util/Debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	FSwsScale::~FSwsScale()
	{
		Release();
	}

	SwsContext* FSwsScale::Alloc(
		const int& n_nInputWidth, 
		const int& n_nInputHeight, 
		const AVPixelFormat& n_eInputPixelFormat, 
		const int& n_nOutputWidth, 
		const int& n_nOutputHeight, 
		const AVPixelFormat& n_eOutputPixelFormat)
	{
		Release();

		Context = sws_getContext(
			n_nInputWidth, n_nInputHeight, n_eInputPixelFormat,
			n_nOutputWidth, n_nOutputHeight, n_eOutputPixelFormat,
			SWS_BICUBIC, nullptr, nullptr, nullptr);

		ThrowExceptionExpr(!Context, "Fail to alloc sws context.\n");

		int ret = av_image_alloc(CaleData, LineSize,
			n_nOutputWidth, n_nOutputHeight, n_eOutputPixelFormat, 32);

		ThrowExceptionCodeExpr(ret < 0, ret, "Fail to allocate destination image");

		m_nInputWidth = n_nInputWidth;
		m_nInputHeight = n_nInputHeight;
		m_eInputPixelFormat = n_eInputPixelFormat;
		m_nOutputWidth = n_nOutputWidth;
		m_nOutputHeight = n_nOutputHeight;
		m_eOutputPixelFormat = n_eOutputPixelFormat;

		return Context;
	}

	SwsContext* FSwsScale::Alloc(AVCodecContext* n_InputCodecContext, 
		AVCodecContext* n_OutputCodecContext)
	{
		return Alloc(
			n_InputCodecContext->width, 
			n_InputCodecContext->height, 
			n_InputCodecContext->pix_fmt,
			n_OutputCodecContext->width, 
			n_OutputCodecContext->height, 
			n_OutputCodecContext->pix_fmt);
	}

	void FSwsScale::Release()
	{
		if (CaleData[0])
			av_freep(&CaleData[0]);
		memset(LineSize, 0, sizeof(LineSize));

		if (Context)
		{
			sws_freeContext(Context);
			Context = nullptr;
		}
	}

	int FSwsScale::Scale(uint8_t** n_InputData, const int* n_InputLineSize)
	{
		return Scale(n_InputData, n_InputLineSize, CaleData, LineSize);
	}

	int FSwsScale::Scale(uint8_t** n_InputData, const int* n_InputLineSize,
		uint8_t** n_OutputData, const int* n_OuputLineSize)
	{
		if (!Context) return 0;

		ThrowExceptionExpr(!CaleData[0], 
			"You should call function Alloc() first.\n");

		return sws_scale(Context, n_InputData, n_InputLineSize, 0, 
			m_nInputHeight, n_OutputData, n_OuputLineSize);
	}

	int FSwsScale::Scale(const AVFrame* n_InputFrame)
	{
		return Scale((uint8_t**)n_InputFrame->data, n_InputFrame->linesize);
	}

}

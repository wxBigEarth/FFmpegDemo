#ifndef __CODECCONTEXT_H__
#define __CODECCONTEXT_H__
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "Util/Setting.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	// The CodecId that supported hardware accel
	struct FHwCodec
	{
		AVCodecID	Id = AVCodecID::AV_CODEC_ID_NONE;
		const char* Name = nullptr;
		bool		IsDecoder = false;
		int			GraphicCard = kGraphicCardNvidia;
	};

	struct FCodecContext
	{
		FCodecContext() = default;
		~FCodecContext();

		// Find decode codec by id
		static const AVCodec* FindDecodeCodec(AVCodecID n_CodecID);
		// Find decode codec by name
		static const AVCodec* FindDecodeCodec(const char* n_szName);
		// Find encode codec by id
		static const AVCodec* FindEncodeCodec(AVCodecID n_CodecID);
		// Find encode codec by name
		static const AVCodec* FindEncodeCodec(const char* n_szName);
		// Get all hardware device types
		static const std::vector<std::string> GetHwDeviceTypes();
		// Find Codecs that support hardware accel
		static const std::vector<FHwCodec> FindAllHwCodecs();

		// Alloc codec context memory
		AVCodecContext* Alloc(const AVCodec* n_Codec);
		// Release 
		void Release();

		// Get pixel format for hardware codec
		int GetHwPixelFormat(const AVCodec* n_Codec, AVHWDeviceType n_eHwDeviceType,
			AVPixelFormat& n_ePixelFormat);

		// Open a device of the specified type and create an AVHWDeviceContext
		void InitHardwareContext(AVHWDeviceType n_eHwDeviceType);

		// Copy codec parameter form stream
		void CopyCodecParameter(const AVStream* n_Stream);
		void CopyCodecParameter(const AVCodecContext* n_CodecContext);

		// Open codec context
		void Open(AVDictionary** n_Options = nullptr);

		// Decode AVPacket
		// n_Func return value: 
		int DecodePacket(const AVPacket* n_Packet, 
			std::function<int(AVFrame* n_Frame)> n_Func);
		// Encode AVFrame
		// n_Func return value: 
		int EncodeFrame(const AVFrame* n_Frame,
			std::function<int(AVPacket* n_Packet)> n_Func);

		// Video: Get number of planes by pixel format
		int GetPixFmtPlaneCount();

		// Audio: Check if the sample format is planar.
		int IsSampleFmtPlanar();

		// Audio: number of bytes per sample
		int GetBytesPerSample();

		// Get hardware pixel format of codec
		const AVPixelFormat GetHwPixelFormat() const;

		// for video codec, get final pixel format of codec
		const AVPixelFormat GetPixelFormat() const;

		AVCodecContext* Context = nullptr;

	protected:
		AVPixelFormat	m_eHwPixelFormat = AVPixelFormat::AV_PIX_FMT_NONE;
		AVPixelFormat	m_eGraphicCardPixelFormat = AVPixelFormat::AV_PIX_FMT_NONE;
		AVBufferRef*	m_HwDeviceContext = nullptr;

		AVPacket*		m_Packet = nullptr;
		AVFrame*		m_Frame = nullptr;
		AVFrame*		m_SwFrame = nullptr;
	};


	//////////////////////////////////////////////////////////////////////
	extern "C"
	{
		// return value: 0: the same format
		int CompareCodecFormat(
			std::shared_ptr<FCodecContext> n_InputCodecContext,
			std::shared_ptr<FCodecContext> n_OutputCodecContext);

		// Addition setting for codec context
		void CodecContextAddition(AVCodecContext* n_CodecContext);

		// Video: Get number of planes by pixel format
		int GetPixFmtPlaneCount(AVPixelFormat n_ePixelFormat);

		// Video: Get return a pixel format descriptor for provided pixel format or NULL if 
		// this pixel format is unknown
		const AVPixFmtDescriptor* GetPixFmtDesc(AVPixelFormat n_ePixelFormat);

		// Video: Get the number of bits per pixel used by the pixel format described by pix desc
		int GetBitsPerPixel(const AVPixFmtDescriptor* n_PixDesc);

		// Audio: Check if the sample format is planar.
		int IsSampleFmtPlanar(AVSampleFormat n_eSampleFormat);

		// Audio: number of bytes per sample
		int GetBytesPerSample(AVSampleFormat n_eSampleFormat);

		AVCodecContext* CopyCodecContext(
			const AVCodecContext* n_CodecContext);
	}
}

#endif // __CODECCONTEXT_H__

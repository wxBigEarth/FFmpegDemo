#ifndef __CODECCONTEXT_H__
#define __CODECCONTEXT_H__
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "Core/Setting.h"
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
	enum class ECodecType
	{
		CT_None = 0,
		CT_Decoder,
		CT_Encoder
	};

	// The CodecId that supported hardware acceleration
	struct FHwCodec
	{
		const AVCodec*	Codec = nullptr;
		const char*		Name = nullptr;
		ECodecType		CodecType = ECodecType::CT_None;
		EGraphicCard	GraphicCard = EGraphicCard::GC_Nvidia;
	};

	struct FCodecContext
	{
		FCodecContext() = default;
		~FCodecContext();

		// Alloc codec context memory
		AVCodecContext* Alloc(const AVCodec* n_Codec,
			std::shared_ptr<FSetting> n_Setting = nullptr);
		// Release 
		void Release();

		// Copy codec parameter form stream
		void CopyCodecParameter(const AVStream* n_Stream);
		void CopyCodecParameter(const AVCodecContext* n_CodecContext,
			std::shared_ptr<FSetting> n_Setting = nullptr);

		// Config hardware acceleration
		void ConfigureHwAccel();

		// Open codec context
		void Open(AVDictionary** n_Options = nullptr);

		// If codec context is open
		const bool IsOpen() const;

		// Decode AVPacket
		// n_Func return value: 
		int DecodePacket(const AVPacket* n_Packet, 
			std::function<int(AVFrame* n_Frame)> n_Func);
		// Encode AVFrame
		// n_Func return value: 
		int EncodeFrame(const AVFrame* n_Frame,
			std::function<int(AVPacket* n_Packet)> n_Func);

		// Get the codec type,it is decoder or encoder
		const ECodecType Type() const;

		// Check if codec context use hardware acceleration
		const bool IsHardwareCodec() const;

		// Video: Get number of planes by pixel format
		int GetPixFmtPlaneCount() const;

		// Audio: Check if the sample format is planar.
		int IsSampleFmtPlanar() const;

		// Audio: number of bytes per sample
		int GetBytesPerSample() const;

		// Get hardware pixel format of codec context, 
		// only valid after open the codec
		const AVPixelFormat GetHwPixelFormat() const;

		// For video codec, get final pixel format of codec. If use hardware
		// acceleration, it will be the pixel format
		// in the hardware device
		const AVPixelFormat GetPixelFormat() const;

		AVCodecContext* Context = nullptr;

	protected:
		// Get pixel format for hardware codec context
		const AVCodecHWConfig* GetHwCodecConfig(
			const AVCodec* n_Codec,
			AVHWDeviceType n_eHwDeviceType,
			unsigned int n_nFlag);

		// Open a device of the specified type and create an AVHWDeviceContext
		void CreateHwContext(AVHWDeviceType n_eHwDeviceType,
			const AVCodecHWConfig* n_HwConfig);

		// Get valid software pixel format
		void GetValidSwFormat(AVBufferRef* n_HwDeviceContext,
			const AVCodecHWConfig* n_HwConfig);

	protected:
		// Setting
		std::shared_ptr<FSetting>	m_Setting = nullptr;

		// The pixel format of the AVFrame in hardware device
		AVPixelFormat	m_eSwPixelFormat = AVPixelFormat::AV_PIX_FMT_NONE;
		AVBufferRef*	m_HwDeviceContext = nullptr;

		AVPacket*		m_Packet = nullptr;
		AVFrame*		m_Frame = nullptr;
		AVFrame*		m_DestFrame = nullptr;
	};


	//////////////////////////////////////////////////////////////////////
	// Find decode codec by id
	const AVCodec* FindDecodeCodec(AVCodecID n_CodecID,
		std::shared_ptr<FSetting> n_Setting = nullptr);
	// Find decode codec by name
	const AVCodec* FindDecodeCodec(const char* n_szName);
	// Find encode codec by id
	const AVCodec* FindEncodeCodec(AVCodecID n_CodecID,
		std::shared_ptr<FSetting> n_Setting = nullptr);
	// Find encode codec by name
	const AVCodec* FindEncodeCodec(const char* n_szName);
	// Get all hardware device types
	const std::vector<std::string> GetHwDeviceTypes();
	// Find Codec list that support hardware acceleration
	const std::vector<FHwCodec> FindAllHwCodecs();

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

#endif // __CODECCONTEXT_H__

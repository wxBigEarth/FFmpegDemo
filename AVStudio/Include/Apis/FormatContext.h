#ifndef __FORMATCONTEXT_H__
#define __FORMATCONTEXT_H__
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	struct FFormatContext
	{
		FFormatContext() = default;
		~FFormatContext();

		// Open the input file
		AVFormatContext* OpenInputFile(const std::string& n_sFile, 
			const AVInputFormat* n_InputFormat = nullptr, 
			AVDictionary* n_Options = nullptr);
		// Alloc output file memory
		AVFormatContext* AllocOutputFile(const std::string& n_sFile,
			const AVOutputFormat* n_OutputFormat = nullptr,
			const char* n_szFormatName = nullptr);
		// Release format context
		void Release();

		// Get the count of stream
		const unsigned int StreamSize();
		// The length of the context
		const double Length();

		// Is this context valid
		const bool IsValid() const;

		// Get stream
		AVStream* FindStream(unsigned int n_nStreamIndex);
		// Get stream index by media type: n_eMediaType
		int FindStreamIndex(AVMediaType n_eMediaType, 
			const AVCodec** n_Codec = nullptr);

		// Get media type by stream index
		const AVMediaType GetStreamMediaType(unsigned int n_nStreamIndex);

		// Create stream for output file
		AVStream* BuildStream(AVCodecContext* n_CodecContext);
		AVStream* BuildStream(AVStream* n_Stream);

		// Read packet from input file
		int ReadPacket(AVPacket* n_Packet) const;

		// Seek to the specified position in the input file
		int SeekFrame(int n_nStreamIndex, int64_t n_nPts, int n_nFlags);

		// open the output file
		void OpenOutputFile();

		// Write header into output file
		void WriteHeader();
		// Write packet into output file
		int InterleavedWritePacket(AVPacket* n_Packet);
		int WritePacket(AVPacket* n_Packet);
		// Write trailer into output file
		void WriteTrailer();

		AVFormatContext*	Context = nullptr;

		// File Name
		std::string			Name;

	protected:
		void RescalePacket(AVPacket* n_Packet);

		bool m_bWriteHeader = false;
	};


	// Get the sample format supported by n_Codec, the n_eSampleFormat comes first
	AVSampleFormat GetSupportedSampleFormat(const AVCodec* n_Codec, 
		enum AVSampleFormat n_eSampleFormat);
	// Get the sample rate supported by n_Codec, select the most closest to n_nSampleRate
	int GetSupportedSampleRate(const AVCodec* n_Codec, int n_nSampleRate);
	// Get layout with the highest channel count
	int GetSupportedChannelLayout(const AVCodec* n_Codec, 
		AVChannelLayout* n_ChannelLayout);

	// Get pixel format supported by n_Codec, the n_ePixelFormat comes first
	AVPixelFormat GetSupportedPixelFormat(const AVCodec* n_Codec, 
		AVPixelFormat n_ePixelFormat);
	// Get the frame rate supported by n_Codec, select the most closest to n_FrameRate
	AVRational GetSupportedFrameRate(const AVCodec* n_Codec, 
		const AVRational& n_FrameRate);

	int FindStreamIndex(AVFormatContext* n_FormatContext, AVMediaType n_eMediaType);

	const AVInputFormat* FindInputFormat(const std::string& n_sShortName);
}

#endif // __FORMATCONTEXT_H__
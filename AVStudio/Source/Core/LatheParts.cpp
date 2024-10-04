#include "Core/LatheParts.h"



namespace avstudio
{
	void FLatheParts::Release()
	{
		nFlag = 0;
		nStreamIndex = -1;
		Duration = 0;

		Stream = nullptr;

		DesireCodecId = AVCodecID::AV_CODEC_ID_NONE;
	}

	AVCodecID FLatheParts::CodecID() const
	{
		AVCodecID Result = AVCodecID::AV_CODEC_ID_NONE;

		if (Codec)
			Result = Codec->Context->codec_id;
		else if (Stream)
			Result = Stream->codecpar->codec_id;

		return Result;
	}

}
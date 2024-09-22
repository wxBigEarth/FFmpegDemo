#include "Core/LatheParts.h"



namespace avstudio
{
	void FLatheParts::Release()
	{
		nShouldDecode = 0;
		nStreamIndex = -1;
		Duration = 0;

		Stream = nullptr;
		Filter = nullptr;

		DesireCodecId = AVCodecID::AV_CODEC_ID_NONE;

		if (Codec)
		{
			delete Codec;
			Codec = nullptr;
		}

		if (Sws)
		{
			delete Sws;
			Sws = nullptr;
		}

		if (Resample)
		{
			delete Resample;
			Resample = nullptr;
		}

		if (FiFo)
		{
			delete FiFo;
			FiFo = nullptr;
		}
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
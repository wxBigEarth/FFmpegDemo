#ifndef __LATHEPARTS_H__
#define __LATHEPARTS_H__
#include "Apis/CodecContext.h"
#include "Apis/AudioFifo.h"
#include "Apis/Resample.h"
#include "Apis/SwsScale.h"
#include "Filter/Filter.h"
#include "Util/Queue.h"
#include "Util/DataItem.h"


namespace avstudio
{
	struct FLatheParts
	{
		// For input context, indicates weather it should be decoded
		// For output context, not use
		int				nFlag = 0;
		// Index of stream, -1: invalid
		int				nStreamIndex = -1;

		/*
		* For output context only
		* If AVStudio create codec context by default automatic,
		* it will choose [DesireCodecId] if it's valid
		* Notice: it may be incompatible with output context
		*/
		AVCodecID		DesireCodecId = AVCodecID::AV_CODEC_ID_NONE;

		// For output context only, Frame duration
		int64_t			Duration = 1;
		// Start pts oft the input context
		int64_t 		StartPts = AVERROR_EOF;
		// Real pts while reading the packet from input context
		int64_t			PacketPts = 0;
		// For input context, pts of AVFrame which decoded from AVPacket
		// For output context, keep the max pts of last input context
		int64_t 		FramePts = 0;

		AVStream*		Stream = nullptr;

		// Codec context
		std::shared_ptr<FCodecContext>	Codec = nullptr;
		// Point to Filter instance
		std::shared_ptr<IFilter>		Filter = nullptr;

		std::shared_ptr<FSwsScale>		Sws = nullptr;
		std::shared_ptr<FResample>		Resample = nullptr;
		std::shared_ptr<FAudioFifo>		FiFo = nullptr;

		void Release();

		AVCodecID CodecID() const;
	};

}


#endif // !__LATHEPARTS_H__

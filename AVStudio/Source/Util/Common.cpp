#include <exception>
#include <iostream>
#include <codecvt>
#include "Util/Common.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
};
#endif

namespace avstudio 
{
	void* AVClone(EDataType n_eDataType, void* n_Data)
	{
		void* Data = nullptr;

		if (!n_Data) return Data;

		if (n_eDataType == EDataType::DT_Frame)
		{
			AVFrame* Frame = av_frame_alloc();
			av_frame_move_ref(Frame, (AVFrame*)n_Data);
			Data = Frame;
		}
		else if (n_eDataType == EDataType::DT_Packet)
		{
			AVPacket* Packet = av_packet_alloc();
			av_packet_move_ref(Packet, (AVPacket*)n_Data);
			Data = Packet;
		}

		return Data;
	}

	std::string AnsiToUtf8(const std::string& n_sSource)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		std::wstring wstr = conv.from_bytes(n_sSource);
		return conv.to_bytes(wstr);
	}

}

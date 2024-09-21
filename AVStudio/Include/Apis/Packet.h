#ifndef __PACKET_H__
#define __PACKET_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	struct FPacket
	{
		FPacket() = default;
		~FPacket();

		// Alloc packet memory
		AVPacket* Alloc();
		// Release 
		void Release();

		// Copy packet. No need to alloc m_Packet
		AVPacket* Clone(const AVPacket* n_Packet);
		// Move packet ref
		void MoveRef(AVPacket* n_Packet);
		// Unref packet
		void UnRef();

		AVPacket* Self = nullptr;
	};
}
#endif // __PACKET_H__


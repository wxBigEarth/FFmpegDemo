#include "Apis/Packet.h"
#include "Util/Debug.h"


namespace avstudio
{
	FPacket::~FPacket()
	{
		Release();
	}

	AVPacket* FPacket::Alloc()
	{
		Self = av_packet_alloc();
		ThrowExceptionExpr(!Self, "Fail to create packet\n");

		return Self;
	}

	void FPacket::Release()
	{
		if (Self)
			av_packet_free(&Self);
	}
	
	AVPacket* FPacket::Clone(const AVPacket* n_Packet)
	{
		Self = av_packet_clone(n_Packet);
		return Self;
	}

	void FPacket::MoveRef(AVPacket* n_Packet)
	{
		if (!Self) Alloc();
		av_packet_move_ref(Self, n_Packet);
	}

	void FPacket::UnRef()
	{
		if (Self) av_packet_unref(Self);
	}


}
#include "Util/MediaMask.h"


namespace avstudio
{
	unsigned char GetMediaMask(int n_eMediaType)
	{
		unsigned char n = 0;

		if (n_eMediaType > -1 && n_eMediaType < 6)
			n = (1 << n_eMediaType);

		return n;
	}

	void CombineMedia(unsigned char& n_nMediaMask, int n_eMediaType)
	{
		n_nMediaMask |= GetMediaMask(n_eMediaType);
	}

	void SeparateMedia(unsigned char& n_nMediaMask, int n_eMediaType)
	{
		auto mask = GetMediaMask(n_eMediaType);
		n_nMediaMask &= ~mask;
	}

	bool IsCompriseMedia(const unsigned char n_nMediaMask, int n_eMediaType)
	{
		return n_nMediaMask & GetMediaMask(n_eMediaType);
	}

}

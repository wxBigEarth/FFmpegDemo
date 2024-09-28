#ifndef __MEDIAMASK_H__
#define __MEDIAMASK_H__


namespace avstudio
{
	// Cover [n_eMediaType] to mask
	unsigned char GetMediaMask(int n_eMediaType);

	// Combine another media
	void CombineMedia(unsigned char& n_nMediaMask, int n_eMediaType);

	// Separate one media
	void SeparateMedia(unsigned char& n_nMediaMask, int n_eMediaType);

	// Does [n_nMediaMask] comprise [n_eMediaType]
	bool IsCompriseMedia(const unsigned char n_nMediaMask, int n_eMediaType);

	// Video mask
	constexpr unsigned char MEDIAMASK_VIDEO = 1;
	// Audio mask
	constexpr unsigned char MEDIAMASK_AUDIO = 2;
	// Video and Audio mask
	constexpr unsigned char MEDIAMASK_AV = MEDIAMASK_VIDEO | MEDIAMASK_AUDIO;
}


#endif //!__MEDIAMASK_H__

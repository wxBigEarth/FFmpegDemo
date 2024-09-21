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
	unsigned int GetStreamMask(unsigned int n_nBaseMask, AVMediaType n_eMediaType)
	{
		return n_nBaseMask | (1 << n_eMediaType);
	}

	std::string AnsiToUtf8(const std::string& n_sSource)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		std::wstring wstr = conv.from_bytes(n_sSource);
		return conv.to_bytes(wstr);
	}

}

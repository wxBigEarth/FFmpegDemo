#include <codecvt>
#include "Util/Common.h"

namespace avstudio 
{
	std::string AnsiToUtf8(const std::string& n_sSource)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		std::wstring wstr = conv.from_bytes(n_sSource);
		return conv.to_bytes(wstr);
	}

}

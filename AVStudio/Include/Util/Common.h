#ifndef __COMMON_H__
#define __COMMON_H__
#include <map>
#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio 
{
	enum class EDataType
	{
		DT_None = 0,
		DT_Packet,
		DT_Frame,
	};

	/*
	* Add new stream with [n_eMediaType]
	* unsigned int n_nBaseMask: 
	*		Current mask, [n_eMediaType] will be merged into [n_nBaseMask]
	*/
	unsigned int GetStreamMask(unsigned int n_nBaseMask, AVMediaType n_eMediaType);

	std::string AnsiToUtf8(const std::string& n_sSource);

	template<typename T, typename R>
	void ReleaseMap(std::map<T, R*>& n_mObj)
	{
		for (auto itr = n_mObj.begin();itr != n_mObj.end(); itr++)
		{
			delete itr->second;
			itr->second = nullptr;
		}

		n_mObj.clear();
	}

	template<typename T>
	void ReleaseVector(std::vector<T*>& n_vObj)
	{
		for (size_t i = 0; i < n_vObj.size(); i++)
		{
			delete n_vObj[i];
			n_vObj[i] = nullptr;
		}

		n_vObj.clear();
	}
}

#endif // !__COMMON_H__

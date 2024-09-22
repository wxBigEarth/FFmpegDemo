#ifndef __IOHANDLE_H__
#define __IOHANDLE_H__
#include <list>
#include <mutex>
#include <functional>
#include "Util/Common.h"
#include "Util/DataItem.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	class IIOHandle
	{
	public:
		IIOHandle();
		virtual ~IIOHandle();

		void Release();

		// Writes data into buffer list, and call function [ReceiveData] 
		// to use the data
		int WriteData(const AVMediaType n_eMediaType,
			EDataType n_eDataType, void* n_Data);

		// When writing data, it will call this function
		virtual int ReceiveData();

		virtual size_t GetBufferSize(const AVMediaType n_eMediaType);

		// Getting data from the buffer list
		// Return value: is fetching over
		bool FetchData(std::function<void(FDataItem*)> n_func);

		// Does get last data of Video/Audio stream
		bool IsOver();

	protected:
		/*
		* Check is stream n_eMediaType end
		* return value:
		*	0: waiting for more data
		*	1: contains data
		*	AVERROR__EOF: is end now
		*/
		int IsEnd(const AVMediaType n_eMediaType);
		void ApplyData(const AVMediaType n_eMediaType,
			std::function<void(FDataItem*)> n_func);

	protected:
		// Video list
		std::list<FDataItem*> m_lstVideo;
		std::list<FDataItem*>::iterator m_itrVideo;
		// Audio list
		std::list<FDataItem*> m_lstAudio;
		std::list<FDataItem*>::iterator m_itrAudio;

		std::mutex	_mutex;
	};
}



#endif // !__IOHANDLE_H__

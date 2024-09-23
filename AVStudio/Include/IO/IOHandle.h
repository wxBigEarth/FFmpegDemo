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
		virtual int ReceiveData(const AVMediaType n_eMediaType);

		size_t GetBufferSize(const AVMediaType n_eMediaType);

		// Getting data from the buffer list, the audio and video is
		// Synchronized, and it will call [m_func] to do with the data
		// AVStudio calls it to generate output file
		void AVSync();

		// Pop data from queue, The return value should be free by AVFreeDataPtr()
		FDataItem* PopData(const AVMediaType n_eMediaType);
		/*
		* Check is stream n_eMediaType end
		* return value:
		*	0: waiting for more data
		*	1: contains data
		*	AVERROR_EOF: is end now
		*/
		int IsEnd(const AVMediaType n_eMediaType);

		// Set callback function to do with the data
		void SetupCallback(std::function<void(FDataItem*)> n_func);

	protected:
		// Pop data item and do with callback [n_func]
		// If [n_func] is nullptr, just pop data
		void ApplyData(const AVMediaType n_eMediaType,
			std::function<void(FDataItem*)> n_func);

	private:
		// Video list
		std::list<FDataItem*> m_lstVideo;
		std::list<FDataItem*>::iterator m_itrVideo;
		// Audio list
		std::list<FDataItem*> m_lstAudio;
		std::list<FDataItem*>::iterator m_itrAudio;

		std::mutex	_mutex;

		// Callback when read data
		std::function<void(FDataItem*)> m_func = nullptr;
	};
}



#endif // !__IOHANDLE_H__

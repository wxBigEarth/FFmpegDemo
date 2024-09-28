#ifndef __IOSYNCAV_H__
#define __IOSYNCAV_H__
#include "Util/Queue.h"
#include "IO/IOHandle.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	class CIOSyncAV : public IIOHandle
	{
	public:
		CIOSyncAV();
		virtual ~CIOSyncAV();

		// Override this function to do with data
		int ReceiveData(const AVMediaType n_eMediaType,
			EDataType n_eDataType, void* n_Data) override;

		// Get the size of buffer
		size_t GetBufferSize(const AVMediaType n_eMediaType) override;

		// Processing data manual
		void DataProcess() override;

		// Getting data from the buffer list, the audio and video is
		// Synchronized, and it will call [m_func] to do with the data
		// AVStudio calls it to generate output file
		void Synchronize();

		void Release();

	protected:
		void ClearBuffer(const AVMediaType n_eMediaType);

	private:
		// Video list
		Queue<FDataItem*>	m_qVideo;
		FDataItem*			m_vItem = nullptr;
		// Audio list
		Queue<FDataItem*>	m_qAudio;
		FDataItem*			m_aItem = nullptr;

		/*
		* Indicate if the stream is coming data
		*/
		unsigned char		m_nReady = 0;
	};
}



#endif // !__IOSYNCAV_H__

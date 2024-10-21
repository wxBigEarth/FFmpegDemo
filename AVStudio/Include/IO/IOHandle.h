#ifndef __IOHANDLE_H__
#define __IOHANDLE_H__
#include <functional>
#include "Util/DataItem.h"


namespace avstudio
{
	class IIOHandle
	{
	public:
		virtual ~IIOHandle();

		/*
		* Init handle
		* unsigned int n_nMediaMask: Media mask, Indicate which stream is selected
		*/
		virtual void Init(unsigned int n_nMediaMask);

		// Do something before start of each context group
		virtual void Processing();

		// Others write data in, and then do something with the data
		// If [n_Data] is PCM data, n_nSize should be the length of data
		virtual int WriteData(const AVMediaType n_eMediaType,
			EDataType n_eDataType, void* n_Data, const int n_nSize = 0);

		// Override this function to do with data
		virtual int ReceiveData(const AVMediaType n_eMediaType,
			EDataType n_eDataType, void* n_Data);

		// Get the size of buffer
		virtual size_t GetBufferSize(const AVMediaType n_eMediaType);

		// Processing data manual
		virtual void DataProcess();

		// Release memory
		virtual void Release();

		// Force stop all stream, set the status to EIOStatus::IO_Done
		virtual void ForceStop();

		const bool IsAllStreamArrived();
		const bool IsAllStreamDone() const;

		// Set callback function to do with the data
		void SetupDateCallback(std::function<void(FDataItem*)> n_func);

		// Set callback function when stopped, Set by AVStudio
		void SetupStopCallback(std::function<void()> n_func);

		// Set if the coming data should be clone
		void SetDataClone(bool n_bClone);

	protected:
		// Check if media type [n_eMediaType] in use
		bool CheckMask(const AVMediaType n_eMediaType);
		// When data arrived, do something
		void DataArrived(const AVMediaType n_eMediaType);

		enum class EIOStatus
		{
			// Waiting data arrive
			IO_Wait = 0,
			// Data is on the way
			IO_Doing,
			// Data is over
			IO_Done
		};

	protected:
		// Status of video data 
		EIOStatus		m_evStatus = EIOStatus::IO_Done;
		// Status of audio data 
		EIOStatus		m_eaStatus = EIOStatus::IO_Done;

		// Indicate which stream in use
		unsigned char	m_nMediaMask = 0;

		// If the coming data should be clone
		bool			m_bClone = true;

		// Callback when read data
		std::function<void(FDataItem*)> m_fnReadData = nullptr;
		// Callback when force stop
		std::function<void()>			m_fnStop = nullptr;
	};
}



#endif // !__IOHANDLE_H__

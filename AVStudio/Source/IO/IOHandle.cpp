#include "IO/IOHandle.h"
#include "Util/MediaMask.h"


namespace avstudio
{
	IIOHandle::~IIOHandle()
	{

	}

	void IIOHandle::Init(unsigned int n_nMediaMask)
	{
		m_nMediaMask = n_nMediaMask;
	}

	void IIOHandle::Processing()
	{
		if (CheckMask(AVMediaType::AVMEDIA_TYPE_VIDEO))
			m_evStatus = EIOStatus::IO_Wait;
		if (CheckMask(AVMediaType::AVMEDIA_TYPE_AUDIO))
			m_eaStatus = EIOStatus::IO_Wait;
	}

	int IIOHandle::WriteData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data, const int n_nSize)
	{
		DataArrived(n_eMediaType);

		void* Data = n_Data;
		if (m_bClone) Data = AVClone(n_eDataType, n_Data);

		return ReceiveData(n_eMediaType, n_eDataType, Data);
	}

	int IIOHandle::ReceiveData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data)
	{
		return 0;
	}

	size_t IIOHandle::GetBufferSize(const AVMediaType n_eMediaType)
	{
		return 0;
	}
	
	void IIOHandle::DataProcess()
	{
	}

	void IIOHandle::Release()
	{
	}

	void IIOHandle::ForceStop()
	{
		m_evStatus = EIOStatus::IO_Done;
		m_eaStatus = EIOStatus::IO_Done;
	}

	const bool IIOHandle::IsAllStreamArrived()
	{
		return (
				!CheckMask(AVMediaType::AVMEDIA_TYPE_VIDEO)
				|| m_evStatus != EIOStatus::IO_Wait
				) && (
				!CheckMask(AVMediaType::AVMEDIA_TYPE_AUDIO)
				|| m_eaStatus != EIOStatus::IO_Wait
				);
	}

	const bool IIOHandle::IsAllStreamDone() const
	{
		return m_eaStatus == EIOStatus::IO_Done &&
			m_evStatus == EIOStatus::IO_Done;
	}

	void IIOHandle::SetupCallback(std::function<void(FDataItem*)> n_func)
	{
		m_func = n_func;
	}

	void IIOHandle::SetDataClone(bool n_bClone)
	{
		m_bClone = n_bClone;
	}

	bool IIOHandle::CheckMask(const AVMediaType n_eMediaType)
	{
		return IsCompriseMedia(m_nMediaMask, n_eMediaType);
	}

	void IIOHandle::DataArrived(const AVMediaType n_eMediaType)
	{
		if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			// Video data has coming
			if (m_evStatus == EIOStatus::IO_Wait)
				m_evStatus = EIOStatus::IO_Doing;
		}
		else if (n_eMediaType == AVMediaType::AVMEDIA_TYPE_AUDIO)
		{
			// Audio data has coming
			if (m_eaStatus == EIOStatus::IO_Wait)
				m_eaStatus = EIOStatus::IO_Doing;
		}
	}

}
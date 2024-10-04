#include "Util/Setting.h"


namespace avstudio
{
	void FSetting::SetGraphicCardType(EGraphicCard n_eCard)
	{
		eGraphicCard = n_eCard;

		switch (eGraphicCard)
		{
		case EGraphicCard::GC_Nvidia:
			eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA;
			break;
		case EGraphicCard::GC_Amd:
			eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_VAAPI;
			break;
		case EGraphicCard::GC_Intel:
			eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_QSV;
			break;
		default:
			break;
		}
	}

	const EGraphicCard FSetting::GetGraphicCard() const
	{
		return eGraphicCard;
	}

	const AVHWDeviceType FSetting::GetHwDeviceType() const
	{
		return eHwDevice;
	}

}

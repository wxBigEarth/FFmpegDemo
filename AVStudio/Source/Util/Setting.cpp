#include "Util/Setting.h"


namespace avstudio
{
	static FSetting g_Setting;

	FSetting* GetSetting()
	{
		return &g_Setting;
	}

	void FSetting::SetGraphicCardType(int n_nCard)
	{
		nGraphicCard = n_nCard;

		switch (n_nCard)
		{
		case kGraphicCardNvidia:
			eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA;
			break;
		case kGraphicCardAmd:
			eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_VAAPI;
			break;
		case kGraphicCardIntel:
			eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_QSV;
			break;
		default:
			break;
		}
	}

	const int FSetting::GetGraphicCard() const
	{
		return nGraphicCard;
	}

	const AVHWDeviceType FSetting::GetHwDeviceType() const
	{
		return eHwDevice;
	}
}

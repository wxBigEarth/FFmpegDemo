#ifndef __AVSETTING_H__
#define __AVSETTING_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	constexpr auto kGraphicCardNvidia = 0;
	constexpr auto kGraphicCardAmd = 1;
	constexpr auto kGraphicCardIntel = 2;

	struct FSetting
	{
		// should enable hardware accel
		bool	bEnableHwAccel = false;

		// Set the type of graphic card
		void SetGraphicCardType(int n_nCard);

		const int GetGraphicCard() const;

		const AVHWDeviceType GetHwDeviceType() const;

	protected:
		// what kind of graphic card
		int		nGraphicCard = kGraphicCardNvidia;

		AVHWDeviceType eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA;
	};

	FSetting* GetSetting();
}
#endif // __AVSETTING_H__


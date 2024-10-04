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
	enum class EGraphicCard
	{
		GC_Nvidia = 0,
		GC_Amd,
		GC_Intel,
	};

	struct FSetting
	{
		// should enable hardware acceleration
		bool	bEnableHwAccel = false;

		// Set the type of graphic card
		void SetGraphicCardType(EGraphicCard n_eCard);

		const EGraphicCard GetGraphicCard() const;

		const AVHWDeviceType GetHwDeviceType() const;

	protected:
		// what kind of graphic card
		EGraphicCard	eGraphicCard = EGraphicCard::GC_Nvidia;

		AVHWDeviceType	eHwDevice = AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA;
	};
}
#endif // __AVSETTING_H__


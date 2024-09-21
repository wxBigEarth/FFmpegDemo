#ifndef __DEVICE_H__
#define __DEVICE_H__
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavdevice/avdevice.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	struct FDevice
	{
		FDevice() = default;
		~FDevice();

		// Get all video and audio device on you PC
		AVDeviceInfoList* Alloc(const char* n_szShortName, 
			const char* n_szDeviceName = nullptr);

		// Get name from device list by index
		std::string GetDeviceDescription(const unsigned int& n_nIndex) const;

		// Print all device information
		void DebugDevices() const;

		void Release();

		AVDeviceInfoList*		Context = nullptr;
		const AVInputFormat*	InputFormat = nullptr;
	};

	// register all device
	void SetupEditorDevice();
}

#endif // __DEVICE_H__

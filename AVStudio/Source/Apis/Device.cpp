#include "Apis/Device.h"
#include "Util/Debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	FDevice::~FDevice()
	{
		Release();
	}

	AVDeviceInfoList* FDevice::Alloc(const char* n_szShortName, 
		const char* n_szDeviceName /*= nullptr*/)
	{
		Release();

		InputFormat = av_find_input_format(n_szShortName);
		if (!InputFormat) return Context;

		AVDictionary* Options = nullptr;
		// List all the devices
		av_dict_set(&Options, "list_devices", "true", 0);

		int ret = avdevice_list_input_sources(InputFormat, n_szDeviceName,
			Options, &Context);

		//ThrowExceptionCodeExpr(ret < 0, ret, "Fail to list devices.");

		//DebugDevices();

		return Context;
	}

	std::string FDevice::GetDeviceDescription(const unsigned int& n_nIndex) const
	{
		if (!Context || n_nIndex >= (unsigned int)Context->nb_devices)
			return std::string("");

		return std::string(Context->devices[n_nIndex]->device_description);
	}

	void FDevice::DebugDevices() const
	{
		if (!Context) return;

		for (int i = 0;i < Context->nb_devices; i++)
		{
			LogInfo("%-16s%s\n", "Name:", Context->devices[i]->device_name);
			LogInfo("%-16s%s\n", "Description:", Context->devices[i]->device_description);

			for (int j = 0;j < Context->devices[i]->nb_media_types; j++)
			{
				const char* szMedaiType = av_get_media_type_string(
					Context->devices[i]->media_types[j]);
				LogInfo("%-16s%s\n", "Media Type:", szMedaiType);
			}

			LogInfo("--------------------------------------------------------\n\n");
		}
	}

	void FDevice::Release()
	{
		avdevice_free_list_devices(&Context);
		Context = nullptr;
	}

	void SetupEditorDevice()
	{
		avdevice_register_all();
	}

}

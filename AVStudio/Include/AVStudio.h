#ifndef __AVSTUDIO_H__
#define __AVSTUDIO_H__


#pragma comment (lib, "avcodec.lib")
#pragma comment (lib, "avdevice.lib")
#pragma comment (lib, "avfilter.lib")
#pragma comment (lib, "avformat.lib")
#pragma comment (lib, "avutil.lib")
#pragma comment (lib, "swresample.lib")
#pragma comment (lib, "swscale.lib")
#pragma comment (lib, "SDL2.lib")

#include "Util/Common.h"
#include "Util/Debug.h"
#include "Util/Setting.h"
#include "Util/Queue.h"
#include "Util/DataItem.h"
#include "Util/MediaMask.h"

#include "Apis/AudioFifo.h"
#include "Apis/SwsScale.h"
#include "Apis/CodecContext.h"
#include "Apis/Device.h"
#include "Apis/FilterGraph.h"
#include "Apis/FormatContext.h"
#include "Apis/Frame.h"
#include "Apis/Packet.h"
#include "Apis/Resample.h"
#include "Apis/Sdl.h"

#include "Filter/Filter.h"
#include "Filter/AudioMixFilter.h"

#include "IO/IOHandle.h"
#include "IO/IOPcm.h"
#include "IO/IOSyncAV.h"
#include "IO/IOPlayer.h"

#include "Core/LatheParts.h"
#include "Core/WorkShop.h"
#include "Core/Factory.h"
#include "Core/Editor.h"


#endif // !__AVSTUDIO_H__.


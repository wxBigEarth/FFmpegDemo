// FFmpegDemo.cpp: 定义应用程序的入口点。
//

#include "FFmpegDemo.h"
#include "AVStudio.h"

using namespace std;
using namespace avstudio;


class CIOHandle : public IIOHandle
{
public:
	int ReceiveData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data) override
	{
		// do something

		return 0;
	}
};

// Cover one media file to another extension
static void Cover()
{
	try
	{
		CEditor Editor;

#if 1
		auto Input = Editor.OpenInputFile("1.mp4");
		auto Output = Editor.AllocOutputFile("1.avi");
#else
		auto Input = Editor.OpenInputFile("1.avi");
		auto Output = Editor.AllocOutputFile("1-1.mp4");
#endif
// 		Output->SetupMiddleware(
// 			[Input, Output](AVCodecContext* Ctx) {
// 
// 				if (Ctx->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
// 				{
// 					// rotate the video 
// 					auto Filter = std::make_shared<CCombineFilter>();
// 					Filter->Init(Output->VideoParts.Codec);
// 					Filter->AppendFilter("rotate", "a=PI/4", true);
// 
// 					Input->VideoParts.Filter = Filter;
// 				}
// 			}
// 		);

// 		auto Filter = std::make_shared<CCombineFilter>();
// 		Filter->Init(Output->VideoParts.Codec);
// 		Filter->AddFilter("vflip", nullptr, true);
// 
// 		Input->VideoParts.Filter = Filter;
		// CEditor will create codec and stream as need
		// if they are not exists.

		// Tip 1: Caller can set codec id as desire like below
		// Notice that it may be incompatible with output context
		// 
		// Output->AudioParts.DesireCodecId = AVCodecID::AV_CODEC_ID_WMAV2;

		// Tip 2: Caller can setup middle ware like below
		// In the middle ware, caller can modify the default parameters of 
		// codec context
		//
		// Output->SetupMiddleware(
		//	[](AVCodecContext* n_Codec) {
		//
		//		n_Codec->framerate = { 30, 1 };
		//		n_Codec->width = 800;
		//		n_Codec->height = 600;
		//	}
		//);

		// Tip 3: Caller can set customize IOHandle like below
		// 
		// CIOHandle ioHandle;
		// Editor.SetIoHandle(&ioHandle);
		// ioHandle.SetupCallback(
		//	 [](FDataItem* n_DataItem)
		//	 {
		//		 // do something. Eg: get PCM data
		//	 }
		// );

		// =======================================================================

		// Or caller can create codec and stream like below
		// and set the parameters
		// 
		// 1. Enable video stream
		//Output->EnableStream(AVMediaType::AVMEDIA_TYPE_VIDEO);

		// 2. Then build codec about the stream
		// auto vctx = Output->BuildCodecContext(
		//	Output->FormatContext()->oformat->video_codec, 
		//	Input->VideoParts.Codec->Context);
		
		// 2-1. Equivalent to this function
		// auto vctx = Output->BuildDefaultCodecContext(
		// 	AVMediaType::AVMEDIA_TYPE_VIDEO,
		// 	Input->VideoParts.Codec->Context);

		// 3. Set codec parameter manual
		//vctx->Context->framerate = Input->VideoParts.Stream->r_frame_rate;
		//vctx->Context->gop_size = 0;

		// 4 Open codec context
		// Output->OpenCodecContext(AVMediaType::AVMEDIA_TYPE_VIDEO);

		// 5. Create stream
		// Output->BuildStream(vctx->Context, AVMediaType::AVMEDIA_TYPE_VIDEO);

		// 1. Enable audio stream
		//Output->EnableStream(AVMediaType::AVMEDIA_TYPE_AUDIO);

		// 2. Then build codec about the stream
		//auto actx = Output->BuildCodecContext(
		//	AVCodecID::AV_CODEC_ID_MP3,
		//	Input->VideoParts.Codec->Context);
		
		// Split the input context into fragments
		// It can be used to split input file
		Input->PickupFragment(5, 6.4);
		Input->PickupFragment(16.2, 4.2);

		Editor.StartUntilRunning();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Cover one media file to another extension use hardware acceleration
static void HwCover()
{
	try
	{
		CEditor Editor;

		auto Setting = Editor.GetSetting();
		Setting->bEnableHwAccel = true;

		auto Input = Editor.OpenInputFile("1.avi");
		auto Output = Editor.AllocOutputFile("1-1.mp4");

		Editor.StartUntilRunning();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Merge 2 media files
static void Merge()
{
	try
	{
		CEditor Editor;

		auto Input1 = Editor.OpenInputFile("1.mp4");
		auto Input2 = Editor.OpenInputFile("2.mp4");
		auto Output = Editor.AllocOutputFile("3.mp4");

		Editor.StartUntilRunning();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Detach audio stream from input context
static void DetachAudioStream()
{
	try
	{
		CEditor Editor;

		auto Input = Editor.OpenInputFile("1.mp4", kNO_GROUP, MEDIAMASK_AUDIO);
		auto Output = Editor.AllocOutputFile("1.mp3");

		Editor.StartUntilRunning();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Mix audio 2.mp4 into 1.mp3
static void MixAudio()
{
	try
	{
		CEditor Editor;

		auto input3 = Editor.OpenInputFile("4.mp4");
		auto Input = Editor.OpenInputFile("2.mp4", 0);
		auto Input2 = Editor.OpenInputFile("1.mp3", 0);
		auto Output = Editor.AllocOutputFile("2-2.mp4");

		// Enable audio stream
		Output->EnableStream(AVMediaType::AVMEDIA_TYPE_AUDIO);
		// Create audio codec context
		Output->BuildDefaultCodecContext(
			AVMediaType::AVMEDIA_TYPE_AUDIO,
			Input->AudioParts.Codec->Context);
		// Open audio codec context
		Output->OpenCodecContext(AVMediaType::AVMEDIA_TYPE_AUDIO);

		// Create filter
		auto Filter = std::make_shared<CAudioMixFilter>();
		Filter->Init(Output->AudioParts.Codec);
		Filter->BuildInputFilter(2);

		Input->SetupFilter(AVMediaType::AVMEDIA_TYPE_AUDIO, Filter);
		Input2->SetupFilter(AVMediaType::AVMEDIA_TYPE_AUDIO, Filter);

		Editor.StartUntilRunning();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Play video
static void Play()
{
	try
	{
		CEditor Editor;
		auto Player = std::make_shared<CSdlPlayer>();

		// Frames will be sent to Player
		Editor.SetIoHandle(Player->GetIoHandle());

		// Enable hardware acceleration
		auto Setting = Editor.GetSetting();
		Setting->bEnableHwAccel = true;

		//auto Input = Editor.OpenInputFile("4.mp4", kNO_GROUP, MEDIAMASK_VIDEO);
		auto Input = Editor.OpenInputFile("4.mp4");
		auto Output = Editor.AllocOutputFile("");
		auto Filter = std::make_shared<CCombineFilter>();

		if (Input->VideoParts.Stream)
		{
			Output->EnableStream(AVMediaType::AVMEDIA_TYPE_VIDEO);
			Output->BuildCodecContext(Input->VideoParts.Stream);
			AVCodecContext* ovCodec = Output->VideoParts.Codec->Context;

			ovCodec->width = 800;
			ovCodec->height = 600;
			//ovCodec->pix_fmt = GetSupportedPixelFormat(ovCodec->codec,
			//	AVPixelFormat::AV_PIX_FMT_NV12);

			Filter->Init(Output->VideoParts.Codec);
			Filter->AppendFilter("rotate", "a=PI/4", true);

			Input->VideoParts.Filter = Filter;
		}

		if (Input->AudioParts.Stream)
		{
			Output->EnableStream(AVMediaType::AVMEDIA_TYPE_AUDIO);
			Output->BuildCodecContext(Input->AudioParts.Stream);
			AVCodecContext* aoCodec = Output->AudioParts.Codec->Context;
			//aoCodec->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16P;
		}

		Player->SetMaxLength(Input->Fmt.Length());
		// For NVIDIA hardware acceleration, default pixel format is nv12
		//Player->Init(Output, "Video Player", nullptr, SDL_PIXELFORMAT_NV12);
		Player->Init(Output, "Video Player", nullptr, SDL_PIXELFORMAT_IYUV);

		Editor.StartUntilRunning();
		Player->Start();

		Player->Join();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Recording audio, (in this way, you can also recording video)
static void RecordAudio()
{
	try
	{
		SetupEditorDevice();

		FDevice Device;
		Device.Alloc("dshow");

		// show available devices
		Device.DebugDevices();

		if (!Device.Context) return;

		std::string sDevice = "audio=";
		// Select device
		sDevice += Device.GetDeviceDescription(0);
		std::string sUtf8 = AnsiToUtf8(sDevice);

		CEditor Editor;

		auto Input = Editor.OpenInputFile(sUtf8, kNO_GROUP, 
			MEDIAMASK_AV, Device.InputFormat);
		auto Output = Editor.AllocOutputFile("record.aac");

		Editor.StartUntilRunning();

		std::thread t([&Editor]() {

			// Record 5 seconds
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			Editor.Stop();

			std::cout << "record over" << std::endl;

			});
		t.detach();

		std::cout << "record start" << std::endl;

		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Recording PCM date to output file
class InputCtx : public CIOPcm
{
public:
	// Fill video frame when write PCM data
	virtual void FillVideoFrame(AVFrame* n_Frame,
		const void* n_Data, const int n_nSize) const
	{
		CIOPcm::FillVideoFrame(n_Frame, n_Data, n_nSize);

		int nPlanes = VideoPlanes();

		if (1 < nPlanes)
		{
			for (int i = 0; i < n_Frame->height; i++)
			{
				for (int j = 0; j < n_Frame->width; j++)
				{
					// set the frame date according to source data format
					// ....
				}
			}
		}
	}

	// Override this function to do with data
	int ReceiveData(const AVMediaType n_eMediaType,
		EDataType n_eDataType, void* n_Data) override
	{
		return Editor.WriteFrame((AVFrame*)n_Data, n_eMediaType, 0);
	}

	void Start(int n_nWidth, int n_nHeight, int n_nFps)
	{
		try
		{
			auto Setting = Editor.GetSetting();

			// Create an empty input context
			auto Input = Editor.OpenInputFile("", kNO_GROUP, MEDIAMASK_AV);
			auto Output = Editor.AllocOutputFile("output.mp4");

			// Build decode codec for input context
			// Maybe the Pixel Format/ Sample Format of the source data 
			// is not the same as output context
			// So the decode codec is need to build a converter 
			// The decode codec is built base on source data
			// The following is example.

			// Video Codec Context
			Input->VideoParts.Codec = FCodecContext::BuildDecodeCodec(
				AVCodecID::AV_CODEC_ID_RAWVIDEO, Setting);

			AVCodecContext* InputVideoCodec = Input->VideoParts.Codec->Context;
			InputVideoCodec->width = n_nWidth;
			InputVideoCodec->height = n_nHeight;
			InputVideoCodec->bit_rate = InputVideoCodec->width * InputVideoCodec->height;
			// assume that frame rate of source data is 30
			InputVideoCodec->time_base = GetSupportedFrameRate(InputVideoCodec->codec, { 1, n_nFps });
			InputVideoCodec->framerate = av_inv_q(InputVideoCodec->time_base);
			// assume that pixel format of source data is AV_PIX_FMT_RGB24
			InputVideoCodec->pix_fmt = GetSupportedPixelFormat(InputVideoCodec->codec, AVPixelFormat::AV_PIX_FMT_RGB24);

			SetupInputParameter(InputVideoCodec);

			// Audio Codec Context
			Input->AudioParts.Codec = FCodecContext::BuildDecodeCodec(
				AVCodecID::AV_CODEC_ID_FIRST_AUDIO, Setting);

			AVCodecContext* InputAudioCodec = Input->AudioParts.Codec->Context;
			InputAudioCodec->bit_rate = 192000;
			InputAudioCodec->sample_rate = GetSupportedSampleRate(InputAudioCodec->codec, 41000);
			InputAudioCodec->time_base = { 1, InputAudioCodec->sample_rate };
			InputAudioCodec->sample_fmt = GetSupportedSampleFormat(InputAudioCodec->codec, AVSampleFormat::AV_SAMPLE_FMT_S16);
			GetSupportedChannelLayout(InputAudioCodec->codec, &InputAudioCodec->ch_layout);

			SetupInputParameter(InputAudioCodec);

			Editor.StartUntilRunning();
		}
		catch (const std::exception& e)
		{
			cout << e.what() << endl;
		}
	}

	void Stop()
	{
		Editor.Stop();
		Editor.Join();
	}

	CEditor Editor;
};

static void RecordPCM()
{
	InputCtx ctx;

	ctx.Start(800, 600, 30);

	std::cout << "record start" << std::endl;

	// the you can call following function to write frame data
	// write 100 frames data
	int t = 0;
	while (t++ < 100)
	{
		// writing data like below
		ctx.WriteData(AVMediaType::AVMEDIA_TYPE_VIDEO,
			EDataType::DT_None, "", 1024);
		ctx.WriteData(AVMediaType::AVMEDIA_TYPE_AUDIO,
			EDataType::DT_None, "", 1024);
	}

	ctx.Stop();

	std::cout << "record end" << std::endl;
}

#undef main
int main()
{
	auto start = std::chrono::steady_clock::now();
	//SetupEditorLog();

	//Cover();
	//HwCover();
	//Merge();
	//DetachAudioStream();
	//MixAudio();
	//Play();
	//RecordAudio();
	//RecordPCM();

	auto end = std::chrono::steady_clock::now();
	auto tt = end - start;

	cout << "Time cost: " << tt.count() * 1.0f / 1000 / 1000 << " millisecond." << endl;

	return 0;
}

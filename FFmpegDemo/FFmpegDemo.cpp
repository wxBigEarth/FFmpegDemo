// FFmpegDemo.cpp: 定义应用程序的入口点。
//

#include "FFmpegDemo.h"
#include "AVStudio.h"

using namespace std;
using namespace avstudio;


class CIOHandle : public IIOHandle
{
public:
	int ReceiveData() override
	{
		FetchData(
			[this](FDataItem* n_DataItem) {
				// do something with the data
			}
		);

		return 0;
	}
};

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
		//	[](FWorkShop* WorkShop) {
		//
		//		if (WorkShop->VideoParts.Codec)
		//		{
		//			WorkShop->VideoParts.Codec->Context->framerate = { 30, 1 };
		//		}
		//	}
		//);

		// Tip 3: Caller can set customize IOHandle like below
		// 
		// CIOHandle ioHandle;
		// Output->IOHandle = &ioHandle;

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
		//Input->PickupFragment(5, 6.4);
		//Input->PickupFragment(16.2, 4.2);

		Editor.Start();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

static void Merge()
{
	try
	{
		CEditor Editor;

		auto Input1 = Editor.OpenInputFile("1.mp4");
		auto Input2 = Editor.OpenInputFile("2.mp4");
		auto Output = Editor.AllocOutputFile("3.mp4");

		Editor.Start();
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

		auto nStreamMask = GetStreamMask(0, AVMediaType::AVMEDIA_TYPE_AUDIO);

		auto Input = Editor.OpenInputFile("1.mp4", kNO_GROUP, nStreamMask);
		auto Output = Editor.AllocOutputFile("1.mp3");

		Editor.Start();
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
		CAudioMixFilter Filter;
		Filter.Init(Output->AudioParts.Codec->Context);
		Filter.InitGraph(2);

		Input->SetupFilter(AVMediaType::AVMEDIA_TYPE_AUDIO, &Filter);
		Input2->SetupFilter(AVMediaType::AVMEDIA_TYPE_AUDIO, &Filter);

		Editor.Start();
		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

// Play video, just for test, do not be care
// You can inherit IPlayer class to define your player
class OutputPlayer : public IIOHandle
{
public:
	int ReceiveData(const AVMediaType n_eMediaType,
		EDataType n_eType, void* n_Data)
	{
		if (!n_Data) return 0;
		// display frame
		// ...
		AVDebug("Output frame %d, Type %d\n", n_eMediaType, n_eType);
		av_frame_free((AVFrame**)&n_Data);

		return 0;
	}
};

static void Play()
{
	try
	{
		CEditor Editor;
		OutputPlayer op;

		auto Input = Editor.OpenInputFile("1.mp4");
		auto Output = Editor.AllocOutputFile("");

		//AVCodecContext* vCodec = Input.GetCodecContext(EStreamType::ST_Video);
		//AVCodecContext* aCodec = Input.GetCodecContext(EStreamType::EST_Audio);

		// Maybe AVFrame decoded from input context should be convert to target 
		// sample format/pixel format, so the target codec context should be created
		//FFormatContext& Output = Editor.GetOutputContext()->GetContext();
		//Output.BuildEncodeCodecContext(Input.FindStream(AVMEDIA_TYPE_VIDEO));
		//Output.BuildEncodeCodecContext(Input.FindStream(AVMEDIA_TYPE_AUDIO));
		//Output.BuildCodecContext(aCodec->codec_id, aCodec);

		Output->EnableStream(AVMediaType::AVMEDIA_TYPE_VIDEO);
		Output->BuildCodecContext(Input->VideoParts.Stream);
		AVCodecContext* ovCodec = Output->VideoParts.Codec->Context;

		ovCodec->width = 800;
		ovCodec->height = 600;
		ovCodec->pix_fmt = GetSupportedPixelFormat(ovCodec->codec, AVPixelFormat::AV_PIX_FMT_YUV420P);

		Output->EnableStream(AVMediaType::AVMEDIA_TYPE_AUDIO);
		Output->BuildCodecContext(Input->AudioParts.Stream);
		AVCodecContext* aoCodec = Output->AudioParts.Codec->Context;
		aoCodec->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S32;

		// Frames will be sent to op
		Output->IOHandle = &op;

		Editor.Start();
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
			kALL_STREAM, Device.InputFormat);
		auto Output = Editor.AllocOutputFile("record.aac");

		Editor.Start();

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
class InputCtx : public IIOPcm
{
public:
	// Fill video frame when write PCM data
	virtual void FillVideoFrame(AVFrame* n_Frame,
		const void* n_Data, const int& n_nSize) const
	{
		IIOPcm::FillVideoFrame(n_Frame, n_Data, n_nSize);

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

	int ReceiveData(const AVMediaType n_eMediaType, AVFrame* n_Frame)
	{
		int ret = 0;

		if (Editor)
		{
			Editor->WriteFrame(n_Frame, n_eMediaType, 0);
		}

		return ret;
	}

	CEditor* Editor = nullptr;
};

static void RecordPCM()
{
	try
	{
		CEditor Editor;
		InputCtx ctx;

		ctx.Editor = &Editor;

		// Create an empty input context
		auto Input = Editor.OpenInputFile("");
		auto Output = Editor.AllocOutputFile("output.mp4");

		// Build decode codec for input context
		// Maybe the Pixel Format/ Sample Format of the source data 
		// is not the same as output context
		// So the decode codec is need to build a converter 
		// The decode codec is built base on source data
		// The following is example.

		// Video Codec Context
		auto vCodec = FCodecContext::FindDecodeCodec(AVCodecID::AV_CODEC_ID_RAWVIDEO);
		Input->VideoParts.Codec = new FCodecContext();

		AVCodecContext* InputVideoCodec = Input->VideoParts.Codec->Alloc(vCodec);
		InputVideoCodec->width = 640;
		InputVideoCodec->height = 480;
		InputVideoCodec->bit_rate = 128000;
		// assume that frame rate of source data is 30
		InputVideoCodec->time_base = GetSupportedFrameRate(InputVideoCodec->codec, { 1, 30 });
		InputVideoCodec->framerate = av_inv_q(InputVideoCodec->time_base);
		// assume that pixel format of source data is AV_PIX_FMT_YUV420P
		InputVideoCodec->pix_fmt = GetSupportedPixelFormat(InputVideoCodec->codec, AVPixelFormat::AV_PIX_FMT_YUV420P);

		ctx.SetupInputParameter(InputVideoCodec);

		// Audio Codec Context
		auto aCodec = FCodecContext::FindDecodeCodec(AVCodecID::AV_CODEC_ID_FIRST_AUDIO);
		Input->AudioParts.Codec = new FCodecContext();

		AVCodecContext* InputAudioCodec = Input->AudioParts.Codec->Alloc(aCodec);
		InputAudioCodec->bit_rate = 192000;
		InputAudioCodec->sample_rate = GetSupportedSampleRate(InputAudioCodec->codec, 41000);
		InputAudioCodec->time_base = { 1, InputAudioCodec->sample_rate };
		InputAudioCodec->sample_fmt = GetSupportedSampleFormat(InputAudioCodec->codec, AVSampleFormat::AV_SAMPLE_FMT_S16);
		GetSupportedChannelLayout(InputAudioCodec->codec, &InputAudioCodec->ch_layout);

		ctx.SetupInputParameter(InputAudioCodec);

		Editor.Start();

		std::cout << "record start" << std::endl;

		// the you can call following function to write frame data
		// write 100 frames datas
		int t = 0;
		while (t++ < 100)
		{
			// writing datas like below
			ctx.Writing(AVMediaType::AVMEDIA_TYPE_VIDEO, "", 1024);
			ctx.Writing(AVMediaType::AVMEDIA_TYPE_AUDIO, "", 1024);
		}

		Editor.Stop();

		std::cout << "record end" << std::endl;

		Editor.Join();
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

int main()
{
	auto start = std::chrono::steady_clock::now();
	//SetupEditorLog();

	//Cover();
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

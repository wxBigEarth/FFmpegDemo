# FFmpegDemo
A video and audio editor library based on ffmpeg 6.0 api;
It can do media cut, media concat, media convert, audio mix, recording.

This project is built on visual studio 2022, and the aveditor project is a static library.
This project is base on FFmpeg 6.0. and SDL 2.30.7

download ffmpeg 6.0:
	https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full-shared.7z
download SDL 2.30.7	
	https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-devel-2.30.7-VC.zip

///////////////////////////////////////////////////////////////////////////////////////////
Modules:

Api: Simple encapsulation of FFmpeg API
SDL: Sdl api that support playing
IO: Input and output of AVFrame or AVPacket
Core: User interface module
Filter: Video and audio filter, User can inherit IFilter class to define new filter.

///////////////////////////////////////////////////////////////////////////////////////////
Class Description:

CEditor: User interface, it will run the tasks in thread. And manage the IO handle and setting module.
	When open an input file, one CFactory object will be created. Caller can open multiple input file
CFactory: Do demuxing, decoding, converting, filtering, encoding. The final data is output from the IO handle.
	There is a input context object, a pointer points to output context, an IO handle pointer in CFactory.
FWorkShop: Input/output context management, including:
			create codec context, stream.
			set Filter.
			manage video/audio pts.
			manage clipping of input context
			and so on.
FLatheParts: Manage the object pointers that required for file demultiplexing, encoding and decoding, conversion, filtering, and other operations.
FSetting: Settings about hardware acceleration on codec

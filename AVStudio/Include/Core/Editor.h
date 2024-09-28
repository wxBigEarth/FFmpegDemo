#ifndef __EDITOR_H__
#define __EDITOR_H__
#include <vector>
#include "Core/Factory.h"
#include "Util/Thread.h"
#include "IO/IOHandle.h"


namespace avstudio
{
	class CEditor : public Thread
	{
	public:
		CEditor();
		~CEditor();

		/*
		* Add an input file, if n_sFileName is empty, then add an empty input file
		* For empty input file, it can record PCM data, and writes into output file
		* 
		* const std::string& n_sFileName:	File name
		* const unsigned int n_nGroupId:	Group id, default is kNO_GROUP
		* const unsigned char n_nMediaMask:	Media mask, Indicate which stream is selected
		*										default is MEDIAMASK_AV
		*/
		FWorkShop* OpenInputFile(const std::string& n_sFileName,
			const unsigned int n_nGroupId = kNO_GROUP,
			const unsigned char n_nMediaMask = MEDIAMASK_AV,
			const AVInputFormat* n_InputFormat = nullptr,
			AVDictionary* n_Options = nullptr);

		// Create an output file, if n_sFileName is empty, 
		// or output context is valid, then get empty output context
		FWorkShop* AllocOutputFile(const std::string& n_sFileName,
			const AVOutputFormat* n_OutputFormat = nullptr,
			const char* n_szFormatName = nullptr);

		// Get the input context by index
		FWorkShop* GetInputContext(const unsigned int n_nIndex);

		// Usually for writing PCM data
		int WriteFrame(AVFrame* n_Frame, AVMediaType n_eMediaType, 
			unsigned int n_nInputIndex = 0);

		// Set the max size of the buffer queue,
		// const size_t n_nSize: 0: ignore limit
		void SetMaxBufferSize(const size_t n_nSize);

		// Setup filter for input context
		void SetupFilter(IFilter* n_Filter, AVMediaType n_eMediaType, 
			unsigned int n_nInputIndex = 0);

		void SetIoHandle(IIOHandle* n_Handle);

		void SetPause(bool n_bPause);

	protected:
		// Before start, do something first
		int Processing();
		int VideoProcessing();
		int AudioProcessing();

		void Run();
		void RunByTurn(const std::vector<size_t>& n_vInputs, bool n_bIsLast);
		void RunByGroup(const std::vector<size_t>& n_vInputs, bool n_bIsLast);

		// Initialize default IOHandle
		void SetupDefaultIoHandle();

		// Number of data in the buffer
		size_t GetBufferSize(AVMediaType n_eMediaType);

	public:
		// If output context is valid, write AVPacket into output file
		void WriteIntoFile(FDataItem* n_DataItem);

	protected:
		void Release();
		void AddLastPts(const double n_dLength);

	private:
		std::vector<CFactory*>	m_vInputCtx;
		FWorkShop				m_OutputFile;

		// IO handle for data stream
		IIOHandle*				m_IoHandle = nullptr;
		// If [m_IoHandle] is set by AVStudio, it should be free when release
		bool					m_bFreeHandle = false;

		bool					m_bPause = false;

		size_t					m_nMaxBufferSize = 50;
	};
}

#endif // !__EDITOR_H__

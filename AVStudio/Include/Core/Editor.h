#ifndef __EDITOR_H__
#define __EDITOR_H__
#include <vector>
#include "Core/Factory.h"
#include "Util/Thread.h"
#include "IO/IOHandle.h"
#include "Core/Setting.h"


namespace avstudio
{
	class CEditor : public Thread
	{
	public:
		CEditor();
		virtual ~CEditor();

		/*
		* Add an input file, if n_sFileName is empty, then add an empty input file
		* For empty input file, it can record PCM data, and writes into output file
		* 
		* const std::string& n_sFileName:	File name
		* const unsigned char n_nGroupId:	Group id, default is kNO_GROUP
		* const unsigned char n_nMediaMask:	Media mask, Indicate which stream 
		*									is selected default is MEDIAMASK_AV
		*/
		std::shared_ptr<FWorkShop> OpenInputFile(
			const std::string& n_sFileName,
			const unsigned char n_nGroupId = kNO_GROUP,
			const unsigned char n_nMediaMask = MEDIAMASK_AV,
			const AVInputFormat* n_InputFormat = nullptr,
			AVDictionary* n_Options = nullptr);

		// Create an output file, if n_sFileName is empty, 
		// or output context is valid, then get empty output context
		std::shared_ptr<FWorkShop> AllocOutputFile(
			const std::string& n_sFileName,
			const AVOutputFormat* n_OutputFormat = nullptr,
			const char* n_szFormatName = nullptr);

		// Get the input context by index
		std::shared_ptr<FWorkShop> GetInputContext(const unsigned int n_nIndex);

		// Get setting object pointer
		std::shared_ptr<FSetting> GetSetting();

		// Usually for writing PCM data
		int WriteFrame(AVFrame* n_Frame, AVMediaType n_eMediaType, 
			unsigned int n_nInputIndex = 0);

		// Set the max size of the buffer queue,
		// const size_t n_nSize: 0: ignore limit
		void SetMaxBufferSize(const size_t n_nSize);

		// Setup filter for input context
		void SetupFilter(std::shared_ptr<IFilter> n_Filter, 
			AVMediaType n_eMediaType,
			unsigned int n_nInputIndex = 0);

		// Setup IO handle, Caller can define what to do with output data
		void SetIoHandle(std::shared_ptr<IIOHandle> n_Handle);

		// Set pause or not
		void SetPause(bool n_bPause);

		// Is pause or not
		const bool IsPause() const;

		// Does start working or not
		const bool IsRunning() const;

		// Start editing and wait until finished processing
		void StartUntilRunning();

	protected:
		// Before start, do something first
		int Processing();
		int VideoProcessing();
		int AudioProcessing();

		void Run();
		void RunByTurn(const std::vector<size_t>& n_vInputs, bool n_bIsLast);
		void RunByGroup(const std::vector<size_t>& n_vInputs, bool n_bIsLast);

		// Initialize default IOHandle
		void SetupIoHandle();

		// Set if input context should be decoded for each input context
		void SetDecodeFlag(int n_nFlag, AVMediaType n_eMediaType);

		// Number of data in the buffer
		size_t GetBufferSize(AVMediaType n_eMediaType);

	public:
		// If output context is valid, write AVPacket into output file
		void WriteIntoFile(FDataItem* n_DataItem);

	protected:
		void Release();
		void AddLastPts(const double n_dLength);

	private:
		std::vector<std::shared_ptr<CFactory>>	m_vInputCtx;
		// Output context
		std::shared_ptr<FWorkShop>				m_Output = nullptr;
		// IO handle for data stream
		std::shared_ptr<IIOHandle>				m_IoHandle = nullptr;
		// Setting
		std::shared_ptr<FSetting>				m_Setting = nullptr;

		// If [m_IoHandle] is set by AVStudio, it should be free when release
		bool									m_bFreeHandle = false;

		// Editor status
		unsigned char							m_nStatus = 0;

		size_t									m_nMaxBufferSize = 30;
	};
}

#endif // !__EDITOR_H__

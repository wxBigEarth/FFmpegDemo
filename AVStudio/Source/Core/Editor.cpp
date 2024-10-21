#include <map>
#include "Core/Editor.h"
#include "Util/Debug.h"
#include "Util/Common.h"
#include "IO/IOSync.h"


namespace avstudio
{
	CEditor::CEditor()
	{
		m_Output = std::make_shared<FWorkShop>();
		m_Setting = std::make_shared<FSetting>();

		m_Output->Init(ECtxType::CT_Output, 0, m_Setting);
	}

	CEditor::~CEditor()
	{
		Release();
	}

	std::shared_ptr<FWorkShop> CEditor::OpenInputFile(
		const std::string& n_sFileName,
		const unsigned char n_nGroupId /*= kNO_GROUP*/,
		const unsigned char n_nMediaMask /*= MEDIAMASK_AV*/,
		const AVInputFormat* n_InputFormat /*= nullptr*/, 
		AVDictionary* n_Options /*= nullptr*/)
	{
		auto Factory = std::make_shared<CFactory>(m_Output);

		auto WorkShop = Factory->Input();
		WorkShop->Fmt.OpenInputFile(n_sFileName, n_InputFormat, n_Options);
		WorkShop->Init(ECtxType::CT_Input, n_nMediaMask, m_Setting);
		WorkShop->SetGroupId(n_nGroupId);

		m_vInputCtx.emplace_back(Factory);

		return WorkShop;
	}

	std::shared_ptr<FWorkShop> CEditor::AllocOutputFile(
		const std::string& n_sFileName,
		const AVOutputFormat* n_OutputFormat /*= nullptr*/, 
		const char* n_szFormatName /*= nullptr*/)
	{
		if (!m_Output->Fmt.IsValid() && !n_sFileName.empty())
		{
			m_Output->Fmt.AllocOutputFile(n_sFileName, 
				n_OutputFormat, n_szFormatName);
			m_Output->Fmt.OpenOutputFile();
		}

		return m_Output;
	}

	std::shared_ptr<FWorkShop> CEditor::GetInputContext(
		const unsigned int n_nIndex)
	{
		std::shared_ptr<FWorkShop> Result = nullptr;

		if (n_nIndex < m_vInputCtx.size())
			Result = m_vInputCtx[n_nIndex]->Input();

		return Result;
	}

	std::shared_ptr<avstudio::FSetting> CEditor::GetSetting()
	{
		return m_Setting;
	}

	int CEditor::WriteFrame(AVFrame* n_Frame, AVMediaType n_eMediaType,
		unsigned int n_nInputIndex /*= 0*/)
	{
		int nResult = -1;

		if (n_nInputIndex < m_vInputCtx.size())
		{
			auto Factory = m_vInputCtx[n_nInputIndex];
			nResult = Factory->WriteFrame(n_Frame, n_eMediaType);
		}

		return nResult;
	}

	void CEditor::SetMaxBufferSize(const size_t n_nSize)
	{
		m_nMaxBufferSize = n_nSize;
	}

	void CEditor::SetupFilter(std::shared_ptr<IFilter> n_Filter, 
		AVMediaType n_eMediaType,
		unsigned int n_nInputIndex /*= 0*/)
	{
		if (!n_Filter) return;

		if (n_nInputIndex < m_vInputCtx.size())
		{
			auto WorkShop = m_vInputCtx[n_nInputIndex]->Input();
			WorkShop->SetupFilter(n_eMediaType, n_Filter);
		}
	}

	void CEditor::SetIoHandle(std::shared_ptr<IIOHandle> n_Handle)
	{
		m_IoHandle = n_Handle;
	}

	void CEditor::SetPause(bool n_bPause)
	{
		m_bPause = n_bPause;
	}

	const bool CEditor::IsPause() const
	{
		return m_bPause;
	}

	int CEditor::Processing()
	{
		if (m_vInputCtx.size() == 0) return -1;

		if (m_Output->IsValid())
		{
			if (VideoProcessing() < 0) return -1;
			if (AudioProcessing() < 0) return -1;
		}
		else
		{
			// Invalid output context, decode anyway
			for (size_t i = 0; i < m_vInputCtx.size(); i++)
			{
				auto Item = m_vInputCtx[i]->Input();
				Item->SetDecodeFlag(1);
			}
		}

		return 0;
	}

	int CEditor::VideoProcessing()
	{
		int nCount = 0;
		std::shared_ptr<FWorkShop> Input = nullptr;

		for (size_t i = 0; i < m_vInputCtx.size(); i++)
		{
			if (m_vInputCtx[i]->Input()->VideoParts.Stream)
			{
				if (!Input) Input = m_vInputCtx[i]->Input();
				nCount++;
			}
		}

		if (!Input) return 0;

		// Is create default codec
		bool bCreate = false;
		// Is input context should be decoded
		bool bDecode = false;

		AVCodecID vDefaultId = m_Output->GetCodecId(
			AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (vDefaultId == AVCodecID::AV_CODEC_ID_NONE) return -1;

		m_Output->EnableStream(AVMediaType::AVMEDIA_TYPE_VIDEO);

		// No video stream
		if (!m_Output->VideoParts.Stream && 
			(Input->VideoParts.Filter ||
				nCount > 1 || 
				Input->VideoParts.CodecID() != vDefaultId))
		{
			// No video codec
			// 1. Filter is valid
			// 2. Number of input > 1
			// 3. Only one input stream, 
			//		but the codec id is not equal to 
			//		default codec id of output video stream
			bCreate = true;
			m_Output->BuildCodecContext(
				vDefaultId, Input->VideoParts.Codec->Context);
		}

		if (m_Output->VideoParts.Codec)
		{
			if (Input->VideoParts.Filter || 
				nCount > 1 ||
				CompareCodecFormat(m_Output->VideoParts.Codec, 
					Input->VideoParts.Codec) != 0)
			{
				bDecode = true;
				m_Output->OpenCodecContext(AVMediaType::AVMEDIA_TYPE_VIDEO);
			}

			for (size_t i = 0; i < m_vInputCtx.size() && bDecode; i++)
			{
				auto Item = m_vInputCtx[i]->Input();
				Item->SetDecodeFlag(1, AVMEDIA_TYPE_VIDEO);
			}
		}

		if (!m_Output->VideoParts.Stream)
		{
			if (bDecode)
			{
				m_Output->BuildStream(
					m_Output->VideoParts.Codec->Context, 
					AVMediaType::AVMEDIA_TYPE_VIDEO);
			}
			else
			{
				// If no need to create video codec, 
				// Build video stream with video stream of input context
				m_Output->BuildStream(Input, 
					AVMediaType::AVMEDIA_TYPE_VIDEO);
			}
		}
		else if (bCreate && m_Output->VideoParts.Codec)
		{
			int ret = avcodec_parameters_from_context(
				m_Output->VideoParts.Stream->codecpar,
				m_Output->VideoParts.Codec->Context);
			ThrowExceptionCodeExpr(ret < 0, ret, 
				"Video: Fail to copy parameters from stream.");

			m_Output->VideoParts.Stream->codecpar->codec_tag = 
				m_Output->VideoParts.Codec->Context->codec_tag;
		}

		return 0;
	}

	int CEditor::AudioProcessing()
	{
		int nCount = 0;
		std::shared_ptr<FWorkShop> Input = nullptr;

		for (size_t i = 0; i < m_vInputCtx.size(); i++)
		{
			if (m_vInputCtx[i]->Input()->AudioParts.Stream)
			{
				if (!Input) Input = m_vInputCtx[i]->Input();
				nCount++;
			}
		}

		if (!Input) return 0;

		// Is create default codec
		bool bCreate = false;
		// Is input context should be decoded
		bool bDecode = false;

		AVCodecID aDefaultId = m_Output->GetCodecId(
			AVMediaType::AVMEDIA_TYPE_AUDIO);
		if (aDefaultId == AVCodecID::AV_CODEC_ID_NONE) return -1;

		m_Output->EnableStream(AVMediaType::AVMEDIA_TYPE_AUDIO);

		// No video stream
		if (!m_Output->AudioParts.Stream &&
			(Input->AudioParts.Filter ||
				nCount > 1 || 
				Input->AudioParts.CodecID() != aDefaultId))
		{
			// No audio codec
			// 1. Filter is valid
			// 2. Number of input > 1
			// 3. Only one input stream, 
			//		but the codec id is not equal to 
			//		default codec id of output video stream
			bCreate = true;
			m_Output->BuildCodecContext(
				aDefaultId, Input->AudioParts.Codec->Context);
		}

		if (m_Output->AudioParts.Codec)
		{
			if (Input->AudioParts.Filter || 
				nCount > 1 ||
				CompareCodecFormat(m_Output->AudioParts.Codec, 
					Input->AudioParts.Codec) != 0)
			{
				bDecode = true;
				m_Output->OpenCodecContext(AVMediaType::AVMEDIA_TYPE_AUDIO);
			}

			for (size_t i = 0; i < m_vInputCtx.size() && bDecode; i++)
			{
				auto Item = m_vInputCtx[i]->Input();
				Item->SetDecodeFlag(1, AVMediaType::AVMEDIA_TYPE_AUDIO);
			}
		}

		if (!m_Output->AudioParts.Stream)
		{
			if (bDecode)
			{
				m_Output->BuildStream(
					m_Output->AudioParts.Codec->Context,
					AVMediaType::AVMEDIA_TYPE_AUDIO);
			}
			else
			{
				// If no need to create audio codec, 
				// Build audio stream with audio stream of input context
				m_Output->BuildStream(Input, 
					AVMediaType::AVMEDIA_TYPE_AUDIO);
			}
		}
		else if (bCreate && m_Output->AudioParts.Codec)
		{
			int ret = avcodec_parameters_from_context(
				m_Output->AudioParts.Stream->codecpar,
				m_Output->AudioParts.Codec->Context);
			ThrowExceptionCodeExpr(ret < 0, ret,
				"Audio: Fail to copy parameters from stream.");

			m_Output->AudioParts.Stream->codecpar->codec_tag =
				m_Output->AudioParts.Codec->Context->codec_tag;
		}

		return 0;
	}

	void CEditor::Run()
	{
		try
		{
			if (Processing() >= 0)
			{
				m_Output->Fmt.WriteHeader();

				SetupIoHandle();

				// Find all groups in input contexts
				std::map<unsigned int, std::vector<size_t>> mGroup;

				for (size_t i = 0; i < m_vInputCtx.size(); i++)
				{
					auto WorkShop = m_vInputCtx[i]->Input();
					auto nGroupId = WorkShop->GetGroupId();

					m_vInputCtx[i]->SetIoHandle(m_IoHandle);

					WorkShop->SetGroupIndex(
						(unsigned int)mGroup[nGroupId].size());
					mGroup[nGroupId].push_back(i);
				}

				// Run by group
				size_t nIndex = 0;

				auto itr = mGroup.find(kNO_GROUP);
				if (itr != mGroup.cend())
				{
					RunByTurn(itr->second, nIndex + 1 == mGroup.size());
					mGroup.erase(itr);
				}

				for (auto itr = mGroup.cbegin(); itr != mGroup.cend(); itr++)
				{
					if (m_IoHandle) m_IoHandle->Processing();
					nIndex++;
					RunByGroup(itr->second, nIndex == mGroup.size());
				}

				m_Output->Fmt.WriteTrailer();
			}
		}
		catch (const std::exception& e)
		{
			LogInfo(e.what());
		}

		Release();
	}

	void CEditor::RunByTurn(const std::vector<size_t>& n_vInputs, bool n_bIsLast)
	{
		int		ret = 0;
		size_t	vSize = 0;
		size_t	aSize = 0;

		for (size_t i = 0; i < n_vInputs.size() && !IsStop(); i++)
		{
			auto& Factory = m_vInputCtx[n_vInputs[i]];
			Factory->Processing(i == m_vInputCtx.size() - 1, n_bIsLast);
			Factory->Input()->CreateFrameConverter(m_Output);

			if (m_IoHandle) m_IoHandle->Processing();

			while (ret != AVERROR_EOF && !IsStop())
			{
				vSize = GetBufferSize(AVMediaType::AVMEDIA_TYPE_VIDEO);
				aSize = GetBufferSize(AVMediaType::AVMEDIA_TYPE_AUDIO);

				if ((m_nMaxBufferSize > 0 &&
					vSize > m_nMaxBufferSize &&
					aSize > m_nMaxBufferSize) || m_bPause)
				{
					std::this_thread::sleep_for(std::chrono::microseconds(10));
					continue;
				}

				ret = Factory->Editing();
			}

			if (IsStop()) Factory->Flush();
			else
			{
				ret = 0;
				Factory->WindUp();

				if (i < n_vInputs.size() - 1)
				{
					// Move AVAudioFifo buffer data to next input context FIFO
					MoveAudioFifoData(Factory->Input()->AudioParts.FiFo,
						m_vInputCtx[n_vInputs[i + 1]]->Input()->AudioParts.FiFo);
				}

				AddLastPts(Factory->Input()->Fmt.Length());
			}

			Factory->Release();
		}
	}

	void CEditor::RunByGroup(const std::vector<size_t>& n_vInputs, bool n_bIsLast)
	{
		int		ret = 0;
		size_t	vSize = 0;
		size_t	aSize = 0;
		unsigned int nEndCount = 0;

		for (size_t i = 0; i < n_vInputs.size() && !IsStop(); i++)
		{
			auto& Factory = m_vInputCtx[n_vInputs[i]];
			Factory->Processing(true, n_bIsLast);
			Factory->Input()->CreateFrameConverter(m_Output);
		}

		while (nEndCount < n_vInputs.size() && !IsStop())
		{
			for (size_t i = 0; i < n_vInputs.size() && !IsStop(); i++)
			{
				auto& Factory = m_vInputCtx[n_vInputs[i]];
				if (Factory->IsEnd()) continue;

				vSize = GetBufferSize(AVMediaType::AVMEDIA_TYPE_VIDEO);
				aSize = GetBufferSize(AVMediaType::AVMEDIA_TYPE_AUDIO);

				if ((m_nMaxBufferSize > 0 &&
					vSize > m_nMaxBufferSize &&
					aSize > m_nMaxBufferSize) || m_bPause)
				{
					std::this_thread::sleep_for(std::chrono::microseconds(10));
					continue;
				}

				if (AVERROR_EOF == Factory->Editing()) nEndCount++;
			}
		}

		double dLength = 0;

		for (size_t i = 0; i < n_vInputs.size(); i++)
		{
			auto& Factory = m_vInputCtx[n_vInputs[i]];

			if (!IsStop())
			{
				if (dLength < Factory->Input()->Fmt.Length())
					dLength = Factory->Input()->Fmt.Length();

				Factory->WindUp();
			}
			else
			{
				Factory->Flush();
			}

			Factory->Release();
		}

		if (!IsStop())
		{
			AddLastPts(dLength);
		}
	}

	void CEditor::SetupIoHandle()
	{
		// If IO handle has been setup, or the output context is invalid, 
		// no need to create default IO handle
		if (!m_IoHandle && m_Output->IsValid())
		{
			m_IoHandle = std::make_shared<CIOSyncAV>();

			m_IoHandle->SetupDateCallback(
				std::bind(&CEditor::WriteIntoFile, this, std::placeholders::_1));

			m_bFreeHandle = true;
		}

		m_IoHandle->Init(m_Output->GetMediaMask());
		m_IoHandle->SetupStopCallback([this]() { Stop(); });
	}

	size_t CEditor::GetBufferSize(AVMediaType n_eMediaType)
	{
		if (!m_IoHandle) return 0;
		return m_IoHandle->GetBufferSize(n_eMediaType);
	}

	void CEditor::WriteIntoFile(FDataItem* n_DataItem)
	{
		if (!m_Output->IsValid()) return;

		if (n_DataItem->DataType == EDataType::DT_Packet)
			m_Output->Fmt.InterleavedWritePacket(n_DataItem->p());
	}

	void CEditor::Release()
	{
		m_vInputCtx.clear();
		m_Output->Release();

		if (m_IoHandle) m_IoHandle->Release();
	}

	void CEditor::AddLastPts(const double n_dLength)
	{
		AVRational tb = { 1, 1 };

		if (m_Output->CheckMedia(AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (m_Output->VideoParts.Codec &&
				m_Output->VideoParts.Codec->Context)
				tb = m_Output->VideoParts.Codec->Context->time_base;
			else if (m_Output->VideoParts.Stream)
				tb = m_Output->VideoParts.Stream->time_base;

			auto v = int64_t(n_dLength / av_q2d(tb));
			m_Output->AddLastPts(v, AVMediaType::AVMEDIA_TYPE_VIDEO);
		}

		if (m_Output->CheckMedia(AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (m_Output->AudioParts.Codec &&
				m_Output->AudioParts.Codec->Context)
				tb = m_Output->AudioParts.Codec->Context->time_base;
			else if (m_Output->AudioParts.Stream)
				tb = m_Output->AudioParts.Stream->time_base;

			auto v = int64_t(n_dLength / av_q2d(tb));
			m_Output->AddLastPts(v, AVMediaType::AVMEDIA_TYPE_AUDIO);
		}
	}

}
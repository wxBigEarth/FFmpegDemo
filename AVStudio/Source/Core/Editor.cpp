#include <map>
#include "Core/Editor.h"
#include "Util/Debug.h"
#include "Util/Common.h"


namespace avstudio
{
	CEditor::CEditor()
	{
	}

	CEditor::~CEditor()
	{
		Release();
	}

	FWorkShop* CEditor::OpenInputFile(const std::string& n_sFileName,
		const unsigned int n_nGroupId /*= kNO_GROUP*/,
		const unsigned int n_nStreamMask /*= kALL_STREAM*/,
		const AVInputFormat* n_InputFormat /*= nullptr*/, 
		AVDictionary* n_Options /*= nullptr*/)
	{
		FWorkShop* AVFile = new FWorkShop();

		if (!n_sFileName.empty())
		{
			AVFile->Fmt.OpenInputFile(n_sFileName, n_InputFormat, n_Options);
		}

		AVFile->Init(ECtxType::CT_Input, n_nStreamMask);
		AVFile->SetGroupId(n_nGroupId);

		size_t nSize = m_vInputCtx.size();

		CFactory* Factory = new CFactory();;
		m_vInputCtx.emplace_back(Factory);
		m_vInputCtx[nSize]->Init(AVFile, &m_OutputFile);

		return AVFile;
	}

	FWorkShop* CEditor::AllocOutputFile(const std::string& n_sFileName,
		const AVOutputFormat* n_OutputFormat /*= nullptr*/, 
		const char* n_szFormatName /*= nullptr*/)
	{
		if (!m_OutputFile.Fmt.IsValid() && !n_sFileName.empty())
		{
			m_OutputFile.Fmt.AllocOutputFile(n_sFileName, 
				n_OutputFormat, n_szFormatName);
			m_OutputFile.Fmt.OpenOutputFile();
		}

		m_OutputFile.Init(ECtxType::CT_Output);

		return &m_OutputFile;
	}

	FWorkShop* CEditor::GetInputContext(const unsigned int n_nIndex)
	{
		FWorkShop* Result = nullptr;

		if (n_nIndex < m_vInputCtx.size())
			Result = m_vInputCtx[n_nIndex]->Input();

		return Result;
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

	void CEditor::SetupFilter(IFilter* n_Filter, AVMediaType n_eMediaType, 
		unsigned int n_nInputIndex /*= 0*/)
	{
		if (!n_Filter) return;

		if (n_nInputIndex < m_vInputCtx.size())
		{
			auto WorkShop = m_vInputCtx[n_nInputIndex]->Input();
			WorkShop->SetupFilter(n_eMediaType, n_Filter);
		}
	}

	int CEditor::Processing()
	{
		if (m_vInputCtx.size() == 0) return -1;

		if (m_OutputFile.IsValid())
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

				Item->CheckForDecoding(&m_OutputFile);
				Item->SetDecodeFlag(1);
			}
		}

		return 0;
	}

	int CEditor::VideoProcessing()
	{
		int nCount = 0;
		FWorkShop* Input = nullptr;

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

		AVCodecID vDefaultId = m_OutputFile.GetCodecId(
			AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (vDefaultId == AVCodecID::AV_CODEC_ID_NONE) return -1;

		m_OutputFile.EnableStream(AVMediaType::AVMEDIA_TYPE_VIDEO);

		// No video stream
		if (!m_OutputFile.VideoParts.Stream && 
			(nCount > 1 || Input->VideoParts.CodecID() != vDefaultId))
		{
			// No video codec
			// More than one input video stream or only one input stream, 
			// but the codec id is not equal to default codec id of output video stream
			bCreate = true;
			m_OutputFile.BuildCodecContext(
				vDefaultId, Input->VideoParts.Codec->Context);
		}

		if (m_OutputFile.VideoParts.Codec)
		{
			if (nCount > 1 ||
				CompareCodecFormat(m_OutputFile.VideoParts.Codec, 
					Input->VideoParts.Codec) != 0)
			{
				bDecode = true;
				m_OutputFile.OpenCodecContext(AVMediaType::AVMEDIA_TYPE_VIDEO);
			}

			for (size_t i = 0; i < m_vInputCtx.size() && bDecode; i++)
			{
				auto Item = m_vInputCtx[i]->Input();

				Item->SetDecodeFlag(
					nCount > 0 ? 1 : Item->AudioParts.nShouldDecode,
					AVMediaType::AVMEDIA_TYPE_VIDEO);
			}
		}

		if (!m_OutputFile.VideoParts.Stream)
		{
			if (bDecode)
			{
				m_OutputFile.BuildStream(
					m_OutputFile.VideoParts.Codec->Context, 
					AVMediaType::AVMEDIA_TYPE_VIDEO);
			}
			else
			{
				// If no need to create video codec, 
				// Build video stream with video stream of input context
				m_OutputFile.BuildStream(Input, AVMediaType::AVMEDIA_TYPE_VIDEO);
			}
		}
		else if (bCreate && m_OutputFile.VideoParts.Codec)
		{
			int ret = avcodec_parameters_from_context(
				m_OutputFile.VideoParts.Stream->codecpar,
				m_OutputFile.VideoParts.Codec->Context);
			ThrowExceptionCodeExpr(ret < 0, ret, 
				"Video: Fail to copy parameters from stream.");

			m_OutputFile.VideoParts.Stream->codecpar->codec_tag = 
				m_OutputFile.VideoParts.Codec->Context->codec_tag;
		}

		return 0;
	}

	int CEditor::AudioProcessing()
	{
		int nCount = 0;
		FWorkShop* Input = nullptr;

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

		AVCodecID aDefaultId = m_OutputFile.GetCodecId(
			AVMediaType::AVMEDIA_TYPE_AUDIO);
		if (aDefaultId == AVCodecID::AV_CODEC_ID_NONE) return -1;

		m_OutputFile.EnableStream(AVMediaType::AVMEDIA_TYPE_AUDIO);

		// No video stream
		if (!m_OutputFile.AudioParts.Stream &&
			(nCount > 1 || Input->AudioParts.CodecID() != aDefaultId))
		{
			// No audio codec
			// More than one input audio stream or only one input stream, 
			// but the codec id is not equal to default codec id of output audio stream
			bCreate = true;
			m_OutputFile.BuildCodecContext(
				aDefaultId, Input->AudioParts.Codec->Context);
		}

		if (m_OutputFile.AudioParts.Codec)
		{
			if (nCount > 1 ||
				CompareCodecFormat(m_OutputFile.AudioParts.Codec, 
					Input->AudioParts.Codec) != 0)
			{
				bDecode = true;
				m_OutputFile.OpenCodecContext(AVMediaType::AVMEDIA_TYPE_AUDIO);
			}

			for (size_t i = 0; i < m_vInputCtx.size() && bDecode; i++)
			{
				auto Item = m_vInputCtx[i]->Input();

				Item->SetDecodeFlag(
					nCount > 0 ? 1 : Item->AudioParts.nShouldDecode,
					AVMediaType::AVMEDIA_TYPE_AUDIO);
			}
		}

		if (!m_OutputFile.AudioParts.Stream)
		{
			if (bDecode)
			{
				m_OutputFile.BuildStream(
					m_OutputFile.AudioParts.Codec->Context,
					AVMediaType::AVMEDIA_TYPE_AUDIO);
			}
			else
			{
				// If no need to create audio codec, 
				// Build audio stream with audio stream of input context
				m_OutputFile.BuildStream(Input, AVMediaType::AVMEDIA_TYPE_AUDIO);
			}
		}
		else if (bCreate && m_OutputFile.AudioParts.Codec)
		{
			int ret = avcodec_parameters_from_context(
				m_OutputFile.AudioParts.Stream->codecpar,
				m_OutputFile.AudioParts.Codec->Context);
			ThrowExceptionCodeExpr(ret < 0, ret,
				"Audio: Fail to copy parameters from stream.");

			m_OutputFile.AudioParts.Stream->codecpar->codec_tag =
				m_OutputFile.AudioParts.Codec->Context->codec_tag;
		}

		return 0;
	}

	void CEditor::Run()
	{
		try
		{
			if (Processing() >= 0)
			{
				m_OutputFile.Processing();
				m_OutputFile.Fmt.WriteHeader();

				int nCurrentId = -1;
				std::vector<int> vInuts;

				// Find all groups in input contexts
				std::map<unsigned int, std::vector<size_t>> mGroup;

				for (size_t i = 0; i < m_vInputCtx.size(); i++)
				{
					auto WorkShop = m_vInputCtx[i]->Input();
					auto nGroupId = WorkShop->GetGroupId();

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
					nIndex++;
					RunByGroup(itr->second, nIndex == mGroup.size());
				}

				m_OutputFile.Fmt.WriteTrailer();
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
			auto Factory = m_vInputCtx[n_vInputs[i]];
			Factory->Processing(i == m_vInputCtx.size() - 1, n_bIsLast);
			Factory->Input()->CheckForDecoding(&m_OutputFile);

			while (ret != AVERROR_EOF && !IsStop())
			{
				vSize = m_OutputFile.GetBufferSize(AVMediaType::AVMEDIA_TYPE_VIDEO);
				aSize = m_OutputFile.GetBufferSize(AVMediaType::AVMEDIA_TYPE_AUDIO);

				if (m_nMaxBufferSize > 0 &&
					vSize > m_nMaxBufferSize &&
					aSize > m_nMaxBufferSize)
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
					// Move AVAudioFifo buffer data to next inut context FIFO
					MoveAudioFifoData(Factory->Input()->AudioParts.FiFo,
						m_vInputCtx[n_vInputs[i + 1]]->Input()->AudioParts.FiFo);
				}

				AddLastPts(Factory->Input()->Fmt.Length());
				m_OutputFile.ResetEndFlag();
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
			auto Factory = m_vInputCtx[n_vInputs[i]];
			Factory->Processing(true, n_bIsLast);
			Factory->Input()->CheckForDecoding(&m_OutputFile);
		}

		while (nEndCount < n_vInputs.size() && !IsStop())
		{
			for (size_t i = 0; i < n_vInputs.size() && !IsStop(); i++)
			{
				auto Factory = m_vInputCtx[n_vInputs[i]];
				if (Factory->IsEnd()) continue;

				vSize = m_OutputFile.GetBufferSize(AVMediaType::AVMEDIA_TYPE_VIDEO);
				aSize = m_OutputFile.GetBufferSize(AVMediaType::AVMEDIA_TYPE_AUDIO);

				if (m_nMaxBufferSize > 0 &&
					vSize > m_nMaxBufferSize &&
					aSize > m_nMaxBufferSize)
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
			auto Factory = m_vInputCtx[n_vInputs[i]];

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
			m_OutputFile.ResetEndFlag();
		}
	}

	void CEditor::Release()
	{
		ReleaseVector(m_vInputCtx);
		m_OutputFile.Release();
	}

	void CEditor::AddLastPts(const double n_dLength)
	{
		AVRational tb = { 1, 1 };

		if (m_OutputFile.IsStreamSelected(AVMediaType::AVMEDIA_TYPE_VIDEO))
		{
			if (m_OutputFile.VideoParts.Codec &&
				m_OutputFile.VideoParts.Codec->Context)
				tb = m_OutputFile.VideoParts.Codec->Context->time_base;
			else if (m_OutputFile.VideoParts.Stream)
				tb = m_OutputFile.VideoParts.Stream->time_base;

			auto v = int64_t(n_dLength / av_q2d(tb));
			m_OutputFile.AddLastPts(v, AVMediaType::AVMEDIA_TYPE_VIDEO);
		}

		if (m_OutputFile.IsStreamSelected(AVMediaType::AVMEDIA_TYPE_AUDIO))
		{
			if (m_OutputFile.AudioParts.Codec &&
				m_OutputFile.AudioParts.Codec->Context)
				tb = m_OutputFile.AudioParts.Codec->Context->time_base;
			else if (m_OutputFile.AudioParts.Stream)
				tb = m_OutputFile.AudioParts.Stream->time_base;

			auto v = int64_t(n_dLength / av_q2d(tb));
			m_OutputFile.AddLastPts(v, AVMediaType::AVMEDIA_TYPE_AUDIO);
		}
	}

}
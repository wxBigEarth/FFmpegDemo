#ifndef __WORKSHOP_H__
#define __WORKSHOP_H__
#include <vector>
#include <functional>
#include "Apis/FormatContext.h"
#include "Core/LatheParts.h"
#include "Util/MediaMask.h"


namespace avstudio
{
	// Default group id
	constexpr unsigned char kNO_GROUP = 0xFF;

	enum class ECtxType
	{
		CT_Input = 0,
		CT_Output,
	};

	struct FWorkShop
	{
		FWorkShop() = default;
		~FWorkShop();

		void Init(ECtxType n_eCtxType, 
			const unsigned char n_nMediaMask = MEDIAMASK_AV,
			std::shared_ptr<FSetting> n_Setting = nullptr);
		void Release();

		// Do something before start
		void Processing();

		/*
		* Input contexts with same id will run at the same time if it 
		* is not kNO_GROUP.
		* Default value of id is kNO_GROUP. At this time, Input contexts will
		* run at first in turn.
		*/
		void SetGroupId(const unsigned char n_nId);
		// Get group id
		const unsigned char GetGroupId() const;

		// Set index in the group. It will auto set by AVStudio.dll
		void SetGroupIndex(const unsigned char n_nIndex);
		// Get index in the group
		const unsigned char GetGroupIndex() const;

		// Is current context valid
		const bool IsValid() const;

		// Return value: Indicate which stream is in use by bit
		const unsigned char GetMediaMask() const;

		// Get current context
		AVFormatContext* FormatContext();

		// Get the codec id automatic, the [DesireCodecId] in LatheParts
		// comes first. (For output context only)
		AVCodecID GetCodecId(AVMediaType n_eMediaType);

		// For output context, select stream
		void EnableStream(AVMediaType n_eMediaType, bool n_bSelect = true);
		// Check if current context comprises stream [n_eMediaType]
		bool CheckMedia(AVMediaType n_eMediaType) const;

		// Build codec context for output context with stream of input context
		std::shared_ptr<FCodecContext> BuildCodecContext(AVStream* n_Stream);
		// Build encode codec context
		std::shared_ptr<FCodecContext> BuildCodecContext(AVCodecID n_eCodecID,
			AVCodecContext* n_InputCodecContext = nullptr);
		// Build encode codec context with default codec id of output context
		std::shared_ptr<FCodecContext> BuildDefaultCodecContext(
			AVMediaType n_eMediaType,
			AVCodecContext* n_InputCodecContext = nullptr);

		// Get stream
		AVStream* FindStream(AVMediaType n_eMediaType);
		// Build streams for output context with input context info
		void BuildStream(std::shared_ptr<FWorkShop> n_Input,
			AVMediaType n_eMediaType = AVMediaType::AVMEDIA_TYPE_UNKNOWN);
		// Build streams for output context with AVCodecContext
		// If n_CodecContext comes from output context, it should be open first
		void BuildStream(AVCodecContext* n_CodecContext,
			AVMediaType n_eMediaType = AVMediaType::AVMEDIA_TYPE_UNKNOWN);

		// Open codec context, if it's output context, it will create stream
		void OpenCodecContext(
			AVMediaType n_eMediaType = AVMediaType::AVMEDIA_TYPE_UNKNOWN);

		// Compare the output codec, check weather it should be decode
		void CreateFrameConverter(std::shared_ptr<FWorkShop> n_Output,
			AVMediaType n_eMediaType = AVMediaType::AVMEDIA_TYPE_UNKNOWN);

		// Set if input context should be decoded, just decoding 
		// without convert frame, for input context only
		void SetDecodeFlag(int n_nFlag, 
			AVMediaType n_eMediaType = AVMediaType::AVMEDIA_TYPE_UNKNOWN);

		// Set filter
		void SetupFilter(AVMediaType n_eMediaType, 
			std::shared_ptr<IFilter> n_Filter);

		// Create Audio FIFO buffer
		void CreateAudioFifo(std::shared_ptr<FCodecContext> n_AudioCodec);

		// Adjust pts for AVFrame that decoded from input context
		int64_t AdjustPts(int64_t n_nPts, AVMediaType n_eMediaType);

		// For output context only, recording the PTS of last input context
		void AddLastPts(int64_t n_nPts, AVMediaType n_eMediaType);
		// Get the PTS of last input context
		int64_t GetLastPts(AVMediaType n_eMediaType) const;

		// Select fragment (in second) for input context, used for file splitting
		void PickupFragment(const double n_dStart, const double n_dLength);

		/*
		* Is [n_nPts] in selected fragment 
		* return value:
		*	>=0: In selected fragment, it is the start pts
		*	-1: Not in selected fragment
		*	AVERROR_EOF: end
		*/
		int64_t TryPickup(int64_t n_nPts, AVMediaType n_eMediaType);

		/*
		* Setup middle ware
		* It will be called when AVStudio create codec automatic,
		* at this time, caller can modify the parameters of codec context
		*/ 
		void SetupMiddleware(std::function<void(AVCodecContext*)> n_func);

		FFormatContext Fmt;

		// Information about video stream
		FLatheParts VideoParts;
		// Information about audio stream
		FLatheParts AudioParts;

	protected:
		struct FFragment
		{
			bool vOk = false;
			bool aOk = false;

			double Start = 0.0;
			// Video Section
			int64_t vTo = 0;

			// Audio Section
			int64_t aTo = 0;

			const bool IsOk() const { return vOk && aOk; }
		};

	protected:
		ECtxType		m_eCtxType = ECtxType::CT_Input;

		// Selected streams
		unsigned char	m_nMediaMask = 0;

		// Input contexts with same id will run at the same time 
		// if it's not -1
		unsigned char	m_nGroupId = kNO_GROUP;
		// Index in the group
		unsigned char	m_nGroupIndex = 0;

		// Count of video AVFrame pts
		int64_t			m_nVideoPts = 0;
		// Count of audio AVFrame pts
		int64_t			m_nAudioPts = 0;

		// Setting
		std::shared_ptr<FSetting>		m_Setting = nullptr;

		// For input context only, used to split input context
		std::vector<FFragment>			m_vFragments;
		// Indicate which fragment is in use
		size_t							m_nFragmentIndex = 0;

		std::function<void(AVCodecContext*)> m_funcMiddleware = nullptr;
	};
}

#endif // !__WORKSHOP_H__

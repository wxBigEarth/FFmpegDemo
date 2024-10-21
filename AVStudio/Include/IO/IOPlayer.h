#ifndef __IOPLAYER_H__
#define __IOPLAYER_H__
#include <thread>
#include <list>
#include <mutex>
#include "IO/IOHandle.h"
#include "Util/Callback.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
	enum class EIOPEventId
	{
		// It should update video frame now
		PEI_UpdateVideo = 0,
		// End of file, it should stop now
		PEI_EOF,
	};

	class CIOPlayer : public IIOHandle
	{
	public:
		CIOPlayer() = default;
		virtual ~CIOPlayer();

		// Others write data in, and then do something with the data
		// If [n_Data] is PCM data, n_nSize should be the length of data
		int WriteData(const AVMediaType n_eMediaType,
			EDataType n_eDataType, void* n_Data, const int n_nSize = 0) override;

		// Override this function to do with data
		int ReceiveData(const AVMediaType n_eMediaType,
			EDataType n_eDataType, void* n_Data) override;

		// Get the size of buffer
		size_t GetBufferSize(const AVMediaType n_eMediaType) override;

		// Do something before start of each context group
		void Processing() override;

		// Release memory
		// Note: IIOHandle::Release() will be called when CEditor done.
		// But maybe player is still running 
		void Release() override;
		void ReleasePlayer();

		// Force stop all stream, set the status to EIOStatus::IO_Done
		void ForceStop() override;

		/*
		* Set callback function
		*/
		void SetupCallback(FCallback<void, EIOPEventId> n_cb);

		// Read video frame from list
		int PopVideo(AVFrame*& n_Frame);
		// Read audio frame from list
		int PopAudio(AVFrame*& n_Frame);

		// Get played duration
		const double PlayedTime() const;

		// Set pause or not
		void SetPause(bool n_bPause);

		// Is pause or not
		const bool IsPause() const;

	protected:
		void Join();

		void PlayProc();

		// Will be trigger when it should update video
		virtual int Update();
		virtual void PlayEnd();

	protected:
		std::thread			m_tPlay;

		std::list<AVFrame*> m_lstVideo;
		std::list<AVFrame*> m_lstAudio;

		AVFrame*			m_vFrame = nullptr;
		AVFrame*			m_aFrame = nullptr;

		std::mutex			m_mutex;

		// The audio time length since playing
		double				m_dAudioTime = 0;
		// The audio time base to double
		double				m_dAudioQ = 0;
		// The video time length since playing
		double				m_dVideoTime = 0;
		// The video time base to double
		double				m_dVideoQ = 0;
		// Indicate pause status
		bool				m_bPause = false;

		// Callback event
		FCallback<void, EIOPEventId>	m_PlayerCb;
	};
}

#endif // !__IOPLAYER_H__

#ifndef __IOPLAYER_H__
#define __IOPLAYER_H__
#include <thread>
#include <list>
#include <mutex>
#include "IO/IOHandle.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif


namespace avstudio
{
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
		void Release() override;

		int PopAudio(AVFrame*& n_Frame);

	protected:
		void Join();

		void PlayVideo();
		void PlayAudio();
		void PlayerEvent();

		// Override this method to display video picture
		virtual int UpdateVideo(AVFrame* n_Frame, const double n_dTimestamp) = 0;
		// Override this method to play audio
		virtual int UpdateAudio(AVFrame* n_Frame, const double n_dTimestamp) = 0;
		virtual void UpdateTime();
		virtual void UpdateEvent();

	protected:
		std::thread m_tVideo;
		std::thread m_tAudio;
		std::thread m_tEvent;

		std::list<AVFrame*> m_lstVideo;
		std::list<AVFrame*> m_lstAudio;

		std::mutex m_mutex;

		// The time length since playing
		double		m_dPlayTime = 0;
	};
}

#endif // !__IOPLAYER_H__

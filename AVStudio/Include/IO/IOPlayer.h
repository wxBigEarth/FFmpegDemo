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

		// Read video frame from list
		int PopVideo(AVFrame*& n_Frame);
		// Read audio frame from list
		int PopAudio(AVFrame*& n_Frame);

		// Get played duration
		const double PlayedTime() const;

	protected:
		void Join();

		void PlayProc();

		// Will be trigger when it should update video
		virtual int Update(double n_dPlayedTime) = 0;

	protected:
		std::thread m_tPlay;

		std::list<AVFrame*> m_lstVideo;
		std::list<AVFrame*> m_lstAudio;

		std::mutex			m_mutex;

		// The audio time length since playing
		double				m_dAudioTime = 0;
		// The audio time base to double
		double				m_dAudioQ = 0;
		// The video time length since playing
		double				m_dVideoTime = 0;
		// The video time base to double
		double				m_dVideoQ = 0;
	};
}

#endif // !__IOPLAYER_H__

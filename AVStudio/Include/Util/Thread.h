#ifndef __THREAD_H__
#define __THREAD_H__
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>


namespace avstudio 
{
	enum class ETimeUnit
	{
		// Second
		TU_Second = 0,
		// Milli second
		TU_Millisecond,
		// Micro second
		TU_Microsecond,
		// Nanosecond
		TU_Nanosecond,
	};

	class Thread
	{
	public:
		Thread();
		virtual ~Thread();

		//base option
		virtual void Start();

		virtual void Stop();

		virtual void Join();

		virtual bool IsStop();

		void Sleep(uint32_t n_nDuration, ETimeUnit n_eSleep = ETimeUnit::TU_Millisecond);

		void SetStartupCallback(std::function<void()> n_func);
		void SetFinishedCallback(std::function<void()> n_func);

	protected:
		void Worker();

		//TO DO
		virtual void Run() = 0;

		void BeginEvent();
		void EndEvent();

	protected:
		Thread(const Thread&) = delete;
		Thread& operator=(const Thread&) = delete;

	protected:
		std::atomic_bool	m_bStop = true;
		std::thread			m_pThread;
		std::mutex			m_mutex;

		std::function<void()> m_funcStartup = nullptr;
		std::function<void()> m_funcFinished = nullptr;
	};

}
#endif // __THREAD_H__


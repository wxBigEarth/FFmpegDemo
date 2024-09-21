#include "Util/Thread.h"

namespace avstudio
{
	Thread::Thread()
	{
	}

	Thread::~Thread()
	{
		Join();
	}

	void Thread::Start()
	{
		Join();

		std::unique_lock<std::mutex> Lock(m_mutex);

		if (m_bStop) {
			m_bStop = false;
			m_pThread = std::thread(std::bind(&Thread::Worker, this));
		}
	}

	void Thread::Stop()
	{
		m_bStop = true;
	}

	void Thread::Join()
	{
		std::unique_lock<std::mutex> Lock(m_mutex);
		if (m_pThread.joinable())
			m_pThread.join();
	}

	bool Thread::IsStop()
	{
		return m_bStop;
	}

	void Thread::Sleep(uint32_t n_nDuration, ETimeUnit n_eSleep /*= ESleep::ES_Millisecond*/)
	{
		switch (n_eSleep)
		{
		case ETimeUnit::TU_Second:
			std::this_thread::sleep_for(std::chrono::seconds(n_nDuration));
			break;
		case ETimeUnit::TU_Millisecond:
			std::this_thread::sleep_for(std::chrono::milliseconds(n_nDuration));
			break;
		case ETimeUnit::TU_Microsecond:
			std::this_thread::sleep_for(std::chrono::microseconds(n_nDuration));
			break;
		case ETimeUnit::TU_Nanosecond:
			std::this_thread::sleep_for(std::chrono::nanoseconds(n_nDuration));
			break;
		}
	}

	void Thread::SetStartupCallback(std::function<void()> n_func)
	{
		m_funcStartup = n_func;
	}

	void Thread::SetFinishedCallback(std::function<void()> n_func)
	{
		m_funcFinished = n_func;
	}

	void Thread::Worker()
	{
		BeginEvent();

		Run();
		Stop();

		EndEvent();
	}

	void Thread::BeginEvent()
	{
		if (m_funcStartup) m_funcStartup();
	}

	void Thread::EndEvent()
	{
		if (m_funcFinished) m_funcFinished();
	}

}

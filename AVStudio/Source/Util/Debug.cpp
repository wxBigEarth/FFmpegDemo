#include <iostream>
#include "Util/Debug.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/log.h>
#include <libavutil/error.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	constexpr size_t BUFFERSIZE_256 = 256;
	constexpr size_t BUFFERSIZE_512 = 512;
	constexpr size_t BUFFERSIZE_2M = 2048;

	char szLogBuffer[BUFFERSIZE_2M] = { 0 };

	static void logCallback(void* avcl, int level, const char* fmt, va_list vl)
	{
		memset(szLogBuffer, 0, sizeof(szLogBuffer));
		std::vsnprintf(szLogBuffer, sizeof(szLogBuffer), fmt, vl);

		//printf_s(szLogBuffer);
		std::cout << szLogBuffer;
	}

	void SetupEditorLog()
	{
		av_log_set_callback(logCallback);
	}

	std::string ErrorCode2String(int n_nErrCode)
	{
		std::string sResult;
		sResult.resize(AV_ERROR_MAX_STRING_SIZE);

		av_make_error_string((char*)sResult.c_str(), 
			AV_ERROR_MAX_STRING_SIZE, n_nErrCode);

		return sResult;
	}

	std::string StringFormat(const char* n_szFunction, 
		int n_nLine, const char* n_szFormat, ...)
	{
		std::string sResult;
		char szBuffer[BUFFERSIZE_256] = { 0 };
		sprintf_s(szBuffer, BUFFERSIZE_256, "%s [%d]: ", n_szFunction, n_nLine);
		sResult = szBuffer;

		if (n_szFormat && strlen(n_szFormat) > 0)
		{
			va_list args;

			va_start(args, n_szFormat);

			char szContent[BUFFERSIZE_512] = { 0 };
			std::vsnprintf(szContent, BUFFERSIZE_512, n_szFormat, args);
			sResult.append(szContent);

			va_end(args);
		}

		return sResult;
	}

	void DebugTimestamp(const AVMediaType n_eMediaType, 
		const int64_t n_nPts, const AVRational& n_Timebase)
	{
		auto t = n_nPts * av_q2d(n_Timebase);
		AVDebug("Stream: %s, Pts: %zd, Timestamp: %lf\n",
			av_get_media_type_string(n_eMediaType), n_nPts, t);
	}

}

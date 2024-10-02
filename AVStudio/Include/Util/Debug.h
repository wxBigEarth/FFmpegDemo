#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef __cplusplus
};
#endif

namespace avstudio
{
	// set up log
	void SetupEditorLog();

	std::string ErrorCode2String(int n_nErrCode);

	// format string with function name and line
	std::string StringFormat(const char* n_szFunction, 
		int n_nLine, const char* n_szFormat, ...);

	void DebugTimestamp(const AVMediaType n_eMediaType,
		const int64_t n_nPts, const AVRational& n_Timebase);

	// if expr is true, throw std::exception with function name and line
#define ThrowException(fmt, ...) \
	{ \
		std::string sException = StringFormat(__FUNCTION__, __LINE__, fmt, __VA_ARGS__); \
		throw std::exception(sException.c_str()); \
	}

	// if expr is true, throw std::exception with function name and line
#define ThrowExceptionExpr(expr, fmt, ...) \
	if (expr) { \
		std::string sException = StringFormat(__FUNCTION__, __LINE__, fmt, __VA_ARGS__); \
		throw std::exception(sException.c_str()); \
	}

	// if expr is true, throw std::exception with function name and line
#define ThrowExceptionCodeExpr(expr, err_code, fmt, ...) \
	if (expr) { \
		std::string sErrMessage = ErrorCode2String(err_code); \
		std::string sException = StringFormat(__FUNCTION__, __LINE__, fmt, __VA_ARGS__); \
		sException.append("  Err: ").append(sErrMessage).append("\n"); \
		throw std::exception(sException.c_str()); \
	}

#define LogInfo(fmt, ...) \
	{ \
		std::string sFormat = StringFormat(__FUNCTION__, __LINE__, nullptr); \
		sFormat.append(fmt); \
		av_log(nullptr, AV_LOG_INFO, sFormat.c_str(), __VA_ARGS__); \
	}

#define LogInfoExpr(expr, fmt, ...) \
	if (expr) { \
		std::string sFormat = StringFormat(__FUNCTION__, __LINE__, nullptr); \
		sFormat.append(fmt); \
		av_log(nullptr, AV_LOG_INFO, sFormat.c_str(), __VA_ARGS__); \
	}

	// debug log info
#ifdef _DEBUG
#define AVDebug(fmt, ...) LogInfo(fmt, __VA_ARGS__)
#else
#define AVDebug(fmt, ...)
#endif // DEBUG


}
#endif // __DEBUG_H__

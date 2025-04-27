
project(AVStudio)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

#aux_source_directory(${PROJECT_SOURCE_DIR}/Source Src)
#aux_source_directory(${PROJECT_SOURCE_DIR}/Source/Util Src)

file(GLOB MainSource 
	${PROJECT_SOURCE_DIR}/Include/*.h
	${PROJECT_SOURCE_DIR}/Source/*.cpp
)

file(GLOB UtilSource 
	${PROJECT_SOURCE_DIR}/Include/Util/*.h
	${PROJECT_SOURCE_DIR}/Source/Util/*.cpp
)

file(GLOB ApiSource 
	${PROJECT_SOURCE_DIR}/Include/Apis/*.h
	${PROJECT_SOURCE_DIR}/Source/Apis/*.cpp
)

file(GLOB SdlSource 
	${PROJECT_SOURCE_DIR}/Include/Sdl/*.h
	${PROJECT_SOURCE_DIR}/Source/Sdl/*.cpp
)

file(GLOB CoreSource 
	${PROJECT_SOURCE_DIR}/Include/Core/*.h
	${PROJECT_SOURCE_DIR}/Source/Core/*.cpp
)

file(GLOB FilterSource 
	${PROJECT_SOURCE_DIR}/Include/Filter/*.h
	${PROJECT_SOURCE_DIR}/Source/Filter/*.cpp
)

file(GLOB IOSource 
	${PROJECT_SOURCE_DIR}/Include/IO/*.h
	${PROJECT_SOURCE_DIR}/Source/IO/*.cpp
)

source_group(Main FILES ${MainSource})
source_group(Util FILES ${UtilSource})
source_group(Apis FILES ${ApiSource})
source_group(Sdl FILES ${SdlSource})
source_group(Core FILES ${CoreSource})
source_group(Filter FILES ${FilterSource})
source_group(IO FILES ${IOSource})

#message("Src: ${Srcs}")

include_directories(Include)
#include_directories(Include/Util)
#include_directories(Include/Apis)
#include_directories(Include/Sdl)
#include_directories(Include/Core)
#include_directories(Include/Filter)
#include_directories(Include/IO)

#add_library(AVStudio SHARED ${Src})
add_library(AVStudio SHARED 
	${MainSource}
	${UtilSource}
	${ApiSource}
	${SdlSource}
	${CoreSource}
	${FilterSource}
	${IOSource}
)

target_include_directories(
	AVStudio
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg
	PUBLIC ${PROJECT_SOURCE_DIR}/../SDL
)

target_link_libraries(
	AVStudio
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/avcodec.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/avdevice.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/avfilter.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/avformat.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/avutil.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/postproc.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/swresample.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../FFmpeg/lib/swscale.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../SDL/lib/SDL2.lib
)

#file(GLOB FFmpegDll ${PROJECT_SOURCE_DIR}/FFmpeg/bin/*.dll)
#file(COPY ${FFmpegDll} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
#file(COPY ${FFmpegDll} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
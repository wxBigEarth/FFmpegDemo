
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

#add_library(${PROJECT_NAME} SHARED ${Src})
add_library(${PROJECT_NAME} SHARED 
	${MainSource}
	${UtilSource}
	${ApiSource}
	${SdlSource}
	${CoreSource}
	${FilterSource}
	${IOSource}
)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/include
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/SDL/include
)

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/avcodec.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/avdevice.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/avfilter.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/avformat.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/avutil.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/postproc.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/swresample.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/FFmpeg/lib/swscale.lib
	PUBLIC ${PROJECT_SOURCE_DIR}/../3rd/SDL/lib/SDL2.lib
)

#file(GLOB FFmpegDll ${PROJECT_SOURCE_DIR}/FFmpeg/bin/*.dll)
#file(COPY ${FFmpegDll} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
#file(COPY ${FFmpegDll} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")

	# 添加自定义命令
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD

		COMMAND ${CMAKE_COMMAND} -E copy 
			${RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.dll
			${INSTALL_SHARED}/${CMAKE_BUILD_TYPE}

		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${ARCHIVE_OUTPUT_DIRECTORY}/${PROJECT_NAME}.lib
			${INSTALL_LIB}/${CMAKE_BUILD_TYPE}

		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${ARCHIVE_OUTPUT_DIRECTORY}/${PROJECT_NAME}.exp
			${INSTALL_LIB}/${CMAKE_BUILD_TYPE}
		
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_CURRENT_LIST_DIR}/Include/
			${INSTALL_ROOT}/include

		COMMENT "Copying library and headers to output directory"
	)

IF (CMAKE_BUILD_TYPE MATCHES "Debug")

	# 添加自定义命令
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD

		COMMAND ${CMAKE_COMMAND} -E copy 
			${RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.pdb
			${INSTALL_SHARED}/${CMAKE_BUILD_TYPE}
	)
endif()

endif()
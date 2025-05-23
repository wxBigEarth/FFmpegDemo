﻿# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("FFmpegDemo")

# 设置可执行程序输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin/Debug)   
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin/Release)

# 设置库文件输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib/Debug)    
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib/Release)

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(INSTALL_ROOT ${PROJECT_SOURCE_DIR}/install)
	set(INSTALL_SHARED ${INSTALL_ROOT}/bin)
	set(INSTALL_LIB ${INSTALL_ROOT}/lib)

	IF (CMAKE_BUILD_TYPE MATCHES "Debug")
        # 创建目标目录结构
        file(MAKE_DIRECTORY ${INSTALL_SHARED}/${CMAKE_BUILD_TYPE})
        file(MAKE_DIRECTORY ${INSTALL_LIB}/${CMAKE_BUILD_TYPE})

		set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
		set(ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG})

	elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        # 创建目标目录结构
        file(MAKE_DIRECTORY ${INSTALL_SHARED}/${CMAKE_BUILD_TYPE})
        file(MAKE_DIRECTORY ${INSTALL_LIB}/${CMAKE_BUILD_TYPE})
	
		set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
		set(ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE})
	endif()
endif()

if (MSVC)
	# 忽略C26812警告
	add_compile_options(/wd26812)
endif()

# 包含子项目。
add_subdirectory ("FFmpegDemo")
add_subdirectory ("AVStudio")

# CMakeList.txt: FFmpegDemo 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。
add_executable (FFmpegDemo "FFmpegDemo.cpp" "FFmpegDemo.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET FFmpegDemo PROPERTY CXX_STANDARD 14)
endif()

include_directories(
	${PROJECT_SOURCE_DIR}/AVStudio/Include
)

# TODO: 如有需要，请添加测试并安装目标。
target_link_libraries(FFmpegDemo PRIVATE AVStudio)
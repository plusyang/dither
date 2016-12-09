#opencv package
set(CMAKE_PREFIX_PATH /usr/local/opencv2)
find_package(OpenCV REQUIRED)

# 增加一个可执行的二进制
ADD_EXECUTABLE( main main.cpp)
TARGET_LINK_LIBRARIES(main ${OpenCV_LIBS})

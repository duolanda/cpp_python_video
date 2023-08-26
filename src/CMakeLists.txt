set(PYTHON_INCLUDE_DIRS /opt/miniconda3/include/python3.9)
set(PYTHON_LIBRARIES /opt/miniconda3/lib/libpython3.9.dylib)

#set(PYTHON_ROOT_DIR /opt/miniconda3)
#find_package(Python REQUIRED COMPONENTS module numpy)

set(CMAKE_PREFIX_PATH "/Users/duolanda/Qt/6.3.1/macos/lib/cmake")
find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED ) #MacOS

add_executable(${PROJECT_NAME} main.cpp VideoWidget.cpp)

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories("/opt/miniconda3/lib/python3.9/site-packages/numpy/core/include")
target_link_libraries(${PROJECT_NAME} ${PYTHON_LIBRARIES} Qt6::Core Qt6::Widgets Qt6::Gui)

add_custom_command(
        TARGET cpp_python_video POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/read_video.py
        ${CMAKE_CURRENT_BINARY_DIR}/read_video.py)

# 复制 exe 和 py
install(TARGETS cpp_python_video DESTINATION bin)
install(FILES read_video.py DESTINATION bin)
install(FILES ${PYTHON_LIBRARIES} DESTINATION bin)




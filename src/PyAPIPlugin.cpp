#include <iostream>
#include <chrono>
#include <thread>

#include <Python.h>
#include <numpy/arrayobject.h>
#include <QApplication>
#include <QTimer>

#include "VideoWidget.h"

#include "PyAPIPlugin.h"

PyAPIPlugin::PyAPIPlugin(QApplication *_app) : app(_app) {}

void PyAPIPlugin::play_audio() {
	std::string command = "ffplay -vn -nodisp -loglevel quiet -i \"../resource/video.mp4\"";
	std::system(command.c_str());
}

int PyAPIPlugin::run()
{
	Py_SetPythonHome(L"../py/.venv");
	Py_Initialize();
	import_array();

	// 导入 Python 模块
	//PyObject *pModule = PyImport_ImportModule("read_video");
	PyObject* pModule = PyImport_ImportModule("video_reader.opencv");

	if (pModule == nullptr) {
		//std::cerr << "Cannot import read_video.py" << std::endl;
		std::cerr << "Cannot import video_reader.opencv" << std::endl;
		return 1;
	}

	// 获取模块中的 init_capture 函数
	PyObject* pInitCapture = PyObject_GetAttrString(pModule, "init_capture");
	if (pInitCapture == nullptr) {
		std::cerr << "Cannot find 'init_capture' function" << std::endl;
		return 1;
	}
	// 获取模块中的 read_video 函数
	PyObject* pReadVideo = PyObject_GetAttrString(pModule, "read_video");
	if (pReadVideo == nullptr) {
		std::cerr << "Cannot find 'read_video' function" << std::endl;
		return 1;
	}
	// 获取模块中的 video_size 函数
	PyObject* pVideoSize = PyObject_GetAttrString(pModule, "video_size");
	if (pVideoSize == nullptr) {
		std::cerr << "Cannot find 'video_size' function" << std::endl;
		return 1;
	}
	// 获取模块中的 video_fps 函数
	PyObject* pVideoFrames = PyObject_GetAttrString(pModule, "video_frames");
	if (pVideoFrames == nullptr) {
		std::cerr << "Cannot find 'video_frames' function" << std::endl;
		return 1;
	}

	// 调用 init_capture 函数
	PyObject* pVideoPath = PyUnicode_FromString("../resource/video.mp4");
	//PyObject *pCap = PyObject_CallObject(pFunc, PyTuple_Pack(1, pVideoPath));
	PyObject* pCap = PyObject_CallObject(pInitCapture, PyTuple_Pack(1, pVideoPath));

	// 检查是否成功打开视频文件
	//PyObject *pIsOpened = PyObject_CallMethod(pCap, "isOpened", nullptr);
	if (pCap == Py_None) {
		std::cerr << "Cannot open the video file" << std::endl;
		return 1;
	}

	// 创建 VideoWidget
	VideoWidget videoWidget;
	videoWidget.show();

	// 根据视频尺寸设置 VideoWidget 尺寸
	PyObject* pSize = PyObject_CallObject(pVideoSize, nullptr);
	PyObject* pWidth = PyTuple_GetItem(pSize, 0);
	PyObject* pHeight = PyTuple_GetItem(pSize, 1);
	//long width = PyLong_AsLong(pWidth);
	//long height = PyLong_AsLong(pHeight);
	int width = static_cast<int>(PyFloat_AsDouble(pWidth));
	if (width < 0 && PyErr_Occurred()) {
		PyErr_Print();
	}
	int height = static_cast<int>(PyFloat_AsDouble(pHeight));
	if (height < 0 && PyErr_Occurred()) {
		PyErr_Print();
	}
	std::cout << "Video Info:" << std::endl;
	std::cout << "width: " << width << " height: " << height << std::endl;
	if (width > 0 && height > 0) {
		videoWidget.setSize(width, height);
	}

	PyObject* pFrameInfo = PyObject_CallObject(pVideoFrames, nullptr);
	PyObject* pFrameCount = PyTuple_GetItem(pFrameInfo, 0);
	PyObject* pFPS = PyTuple_GetItem(pFrameInfo, 1);
	int totalFrames = static_cast<int>(PyFloat_AsDouble(pFrameCount));
	if (totalFrames < 0 && PyErr_Occurred()) {
		PyErr_Print();
	}
	//int fps = PyLong_AsLong(pFPS);
	int fps = static_cast<int>(PyFloat_AsDouble(pFPS));
	if (fps < 0) {
		if (PyErr_Occurred()) PyErr_Print();
		fps = 30;  // 默认 1000 / 30;
	}
	std::cout << "fps: " << fps << " total frames: " << totalFrames << std::endl;
	long long totalSeconds = totalFrames / fps;
	if (totalFrames % fps) {  // 剩余的帧不足一秒，当做一秒算
		totalSeconds += 1;
	}
	std::cout << "Video Duration: " << totalSeconds / 60 << "m" << totalSeconds % 60 << "s (" << totalSeconds << "s)" << std::endl;

	// 创建 QTimer，用于定时更新视频帧
	QTimer timer;
	timer.setInterval(1000 / fps);  // 按 fps 的间隔更新视频帧
	//timer.setInterval(30);  // 按30ms的间隔更新视频帧

	auto now = std::chrono::system_clock::now();
	auto nowNs = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
	auto epochNs = nowNs.time_since_epoch();
	long long timeNs = epochNs.count();
	auto startTime = now;

	long long totalLatency = 0;
	long framePassed = 0;

	PyObject* pRetVal = nullptr;

	// 在 Qt 槽中连接 QTimer 的超时信号
	QObject::connect(&timer, &QTimer::timeout, [&]() {
		if (pRetVal != nullptr) {
			Py_DECREF(pRetVal);
		}
		//PyObject *pRetVal = PyObject_CallMethod(pCap, "read", nullptr);
		pRetVal = PyObject_CallObject(pReadVideo, nullptr);
		PyObject* pIsRead = PyTuple_GetItem(pRetVal, 0);
		PyObject* pFrame = PyTuple_GetItem(pRetVal, 1);
		PyObject* pTimeNs = PyTuple_GetItem(pRetVal, 2);

		if (pIsRead == Py_False) {
			timer.stop();  // 视频播放结束
			Py_DECREF(pRetVal);
			now = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
			long long seconds = duration.count();
			std::cout << "Video Playback Time: " << seconds / 60 << "m" << seconds % 60 << "s (" << seconds << "s)" << std::endl;
			return;
		}

		// 将 Python 的 numpy.array 转换为 QImage
		PyArrayObject* pFrameArray = reinterpret_cast<PyArrayObject*>(pFrame);
		int rows = PyArray_SHAPE(pFrameArray)[0];
		int cols = PyArray_SHAPE(pFrameArray)[1];
		int channels = PyArray_SHAPE(pFrameArray)[2];

		QImage image((uchar*)PyArray_DATA(pFrameArray), cols, rows, QImage::Format_BGR888);

		// 在 VideoWidget 中显示视频帧
		videoWidget.setImage(image);

		now = std::chrono::system_clock::now();
		nowNs = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
		epochNs = nowNs.time_since_epoch();
		timeNs = epochNs.count();
		long long timeNsPY = PyLong_AsLongLong(pTimeNs);
		if (timeNsPY < 0 && PyErr_Occurred()) {
			PyErr_Print();
			// TODO 还不知道怎么处理
		}
		totalLatency += timeNs - timeNsPY;
		framePassed += 1;
		if (framePassed % fps == 0) {
			std::cout << "[Latency (ns)] Total: " << totalLatency << " \tAverage: " << totalLatency / framePassed << " \tframePassed: " << framePassed << std::endl;
		}
		});

	timer.start();
	std::thread audio_thread(play_audio);

	// 运行 Qt 应用
	int ret = app->exec();

	audio_thread.join();

	// 释放 Python 对象
	Py_DECREF(pModule);
	//Py_DECREF(pFunc);
	Py_DECREF(pInitCapture);
	Py_DECREF(pReadVideo);
	Py_DECREF(pVideoSize);
	Py_DECREF(pFrameInfo);
	Py_DECREF(pVideoPath);
	Py_DECREF(pCap);
	Py_DECREF(pSize);
	Py_DECREF(pFPS);
	//Py_DECREF(pIsOpened);

	Py_Finalize();

	return ret;
}


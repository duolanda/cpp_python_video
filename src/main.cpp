#include <iostream>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <QApplication>
#include <QTimer>
#include "VideoWidget.h"

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	Py_Initialize();
	import_array();

	// 导入 Python 模块
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\".\")");
	PyObject *pModule = PyImport_ImportModule("read_video");
	if (pModule == nullptr) {
		std::cerr << "Cannot import read_video.py" << std::endl;
		return 1;
	}

	// 获取模块中的 read_video 函数
	PyObject *pFunc = PyObject_GetAttrString(pModule, "read_video");
	if (pFunc == nullptr) {
		std::cerr << "Cannot find 'read_video' function" << std::endl;
		return 1;
	}

	// 调用 read_video 函数
	PyObject *pVideoPath = PyUnicode_FromString("../resource/video.mp4");
	PyObject *pCap = PyObject_CallObject(pFunc, PyTuple_Pack(1, pVideoPath));

	// 检查是否成功打开视频文件
	PyObject *pIsOpened = PyObject_CallMethod(pCap, "isOpened", nullptr);
	if (pIsOpened == Py_False) {
		std::cerr << "Cannot open the video file" << std::endl;
		return 1;
	}

	// 创建 VideoWidget
	VideoWidget videoWidget;
	videoWidget.show();

	// 创建 QTimer，用于定时更新视频帧
	QTimer timer;
	timer.setInterval(30);  // 按30ms的间隔更新视频帧

	// 在 Qt 槽中连接 QTimer 的超时信号
	QObject::connect(&timer, &QTimer::timeout, [&]() {
		PyObject *pRetVal = PyObject_CallMethod(pCap, "read", nullptr);
		PyObject *pIsRead = PyTuple_GetItem(pRetVal, 0);
		PyObject *pFrame = PyTuple_GetItem(pRetVal, 1);

		if (pIsRead == Py_False) {
			timer.stop();  // 视频播放结束
		}

		// 将 Python 的 numpy.array 转换为 QImage
		PyArrayObject *pFrameArray = reinterpret_cast<PyArrayObject *>(pFrame);
		int rows = PyArray_SHAPE(pFrameArray)[0];
		int cols = PyArray_SHAPE(pFrameArray)[1];
		int channels = PyArray_SHAPE(pFrameArray)[2];

		QImage image((uchar *)PyArray_DATA(pFrameArray), cols, rows, QImage::Format_BGR888);

		// 在 VideoWidget 中显示视频帧
		videoWidget.setImage(image);
		});

	timer.start();

	// 运行 Qt 应用
	int ret = app.exec();

	// 释放 Python 对象
	Py_DECREF(pModule);
	Py_DECREF(pFunc);
	Py_DECREF(pVideoPath);
	Py_DECREF(pCap);
	Py_DECREF(pIsOpened);

	Py_Finalize();

	return ret;
}
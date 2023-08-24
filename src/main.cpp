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

	// ���� Python ģ��
	PyObject *pModule = PyImport_ImportModule("read_video");
	if (pModule == nullptr) {
		std::cerr << "Cannot import read_video.py" << std::endl;
		return 1;
	}

	// ��ȡģ���е� read_video ����
	PyObject *pFunc = PyObject_GetAttrString(pModule, "read_video");
	if (pFunc == nullptr) {
		std::cerr << "Cannot find 'read_video' function" << std::endl;
		return 1;
	}

	// ���� read_video ����
	PyObject *pVideoPath = PyUnicode_FromString("../resource/video.mp4");
	PyObject *pCap = PyObject_CallObject(pFunc, PyTuple_Pack(1, pVideoPath));

	// ����Ƿ�ɹ�����Ƶ�ļ�
	PyObject *pIsOpened = PyObject_CallMethod(pCap, "isOpened", nullptr);
	if (pIsOpened == Py_False) {
		std::cerr << "Cannot open the video file" << std::endl;
		return 1;
	}

	// ���� VideoWidget
	VideoWidget videoWidget;
	videoWidget.show();

	// ���� QTimer�����ڶ�ʱ������Ƶ֡
	QTimer timer;
	timer.setInterval(30);  // ��30ms�ļ��������Ƶ֡

	// �� Qt �������� QTimer �ĳ�ʱ�ź�
	QObject::connect(&timer, &QTimer::timeout, [&]() {
		PyObject *pRetVal = PyObject_CallMethod(pCap, "read", nullptr);
		PyObject *pIsRead = PyTuple_GetItem(pRetVal, 0);
		PyObject *pFrame = PyTuple_GetItem(pRetVal, 1);

		if (pIsRead == Py_False) {
			timer.stop();  // ��Ƶ���Ž���
		}

		// �� Python �� numpy.array ת��Ϊ QImage
		PyArrayObject *pFrameArray = reinterpret_cast<PyArrayObject *>(pFrame);
		int rows = PyArray_SHAPE(pFrameArray)[0];
		int cols = PyArray_SHAPE(pFrameArray)[1];
		int channels = PyArray_SHAPE(pFrameArray)[2];

		QImage image((uchar *)PyArray_DATA(pFrameArray), cols, rows, QImage::Format_RGB888);

		// �� VideoWidget ����ʾ��Ƶ֡
		videoWidget.setImage(image);
		});

	timer.start();

	// ���� Qt Ӧ��
	int ret = app.exec();

	// �ͷ� Python ����
	Py_DECREF(pModule);
	Py_DECREF(pFunc);
	Py_DECREF(pVideoPath);
	Py_DECREF(pCap);
	Py_DECREF(pIsOpened);

	Py_Finalize();

	return ret;
}
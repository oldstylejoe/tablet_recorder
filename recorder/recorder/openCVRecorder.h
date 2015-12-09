//Joe Snider
//7/15
//
//Record from the webcam using openCV
#include <Windows.h>

#include <iostream>
#include <thread>
#include <memory>
#include <atomic>
#include <string>
#include <stdexcept>
#include <tchar.h>

#include "opencv2/opencv.hpp"
#include "SafeFile.h" //to create the file name

#ifndef OPEN_CV_RECORDER
#define OPEN_CV_RECORDER

const int FRAME_RATE = 24;//per second
const int FRAMES_PER_VIDEO = FRAME_RATE * 600; //new video every 10 minutes

using namespace std;

class COpenCVRecorder{
	shared_ptr<thread> m_webcamThread;
	atomic_bool m_shutdown;
	atomic_bool m_going;
	shared_ptr<cv::VideoWriter> m_video;
	shared_ptr<cv::VideoCapture> m_vcap;

	int m_frameWidth;
	int m_frameHeight;

public:
	COpenCVRecorder() {
		m_going = false;
		m_shutdown = false;
	}
	~COpenCVRecorder() {
		m_shutdown = true;
		if (m_webcamThread && m_webcamThread->joinable()) {
			m_webcamThread->join();
		}
	}

public:
	//might hang the thread
	bool isStarted() const { return !m_shutdown; }
	bool isGoing() const { return m_going; }

	void pause() { m_going = false; }
	void unpause() { m_going = true; }

	void stopVideo() {
		if (m_webcamThread && m_webcamThread->joinable()) {
			m_shutdown = true;
			m_webcamThread->join();
			m_webcamThread.reset();
		}
	}

	//restarting is also kosher
	void startVideo() {
		stopVideo();
		try {
			createVideo();
			createFile();
			// start the reader thread (which is responsible for deallocation of everything)
			m_webcamThread.reset(new thread(&COpenCVRecorder::doVideo, this));
		}
		catch (const std::runtime_error &e) {
			m_shutdown = true;
			wchar_t buf[1024];
			size_t t;
			mbstowcs_s(&t, buf, 1024, e.what(), 1024);
			MessageBox(NULL, buf, _T("Error"), MB_ICONERROR | MB_OK);
		}
	}

private:
	//open up the webcam
	void createVideo() {
		m_shutdown = false;
		m_vcap.reset(new cv::VideoCapture(1));
		if (!m_vcap->isOpened()) {
			throw std::runtime_error("Error opening webcam stream");
		}
		m_frameWidth = m_vcap->get(CV_CAP_PROP_FRAME_WIDTH);
		m_frameHeight = m_vcap->get(CV_CAP_PROP_FRAME_HEIGHT);

	}

	//should safely create the file. call from one thread only.
	//could be wonky if the startVideo function is called exactly when this goes.
	//TODO: thread safety.
	void createFile() {
		if (m_video && m_video->isOpened()) {
			m_video->release();
		}
		string fname;
		string base = "c:\\DataLogs\\webcam\\webcam";
		string extension = "avi";
		int id = SafeFile::CreateSafeTimeFileNameNumbered(base, extension, fname);
		if (id < 0) {
			throw std::runtime_error("Error opening video file.\nMore than 1000 created this hour.");
		}

		//m_video.reset(new cv::VideoWriter(fname, CV_FOURCC('M', 'J', 'P', 'G'), FRAME_RATE, cv::Size(m_frameWidth, m_frameHeight), true));
		m_video.reset(new cv::VideoWriter(fname, CV_FOURCC('D', 'I', 'V', 'X'), FRAME_RATE, cv::Size(m_frameWidth, m_frameHeight), true));
		//m_video.reset(new cv::VideoWriter(fname, -1, FRAME_RATE, cv::Size(m_frameWidth, m_frameHeight), true));
		if (!m_video->isOpened()) {
			throw std::runtime_error("Error opening video file stream");
		}

	}

	void doVideo() {
		m_going = true;
		int fontFace = cv::FONT_HERSHEY_PLAIN;
		double fontScale = 2;
		int thickness = 3;
		double timestamp;

		FILETIME ftStart, ftStop;
		char buf[1024];

		cv::Mat frame;
		int frames = 0;
		while (!m_shutdown) {
			GetSystemTimePreciseAsFileTime(&ftStart);
			//m_vcap->operator>>(frame);
			if (m_vcap->grab()) {
				GetSystemTimePreciseAsFileTime(&ftStop);
				m_vcap->retrieve(frame);

				timestamp = m_vcap->get(CV_CAP_PROP_POS_MSEC);
				sprintf_s(buf, "%lu%lu",
					ftStart.dwHighDateTime, ftStart.dwLowDateTime);
				cv::putText(frame, buf, cv::Point(30, 30), fontFace, fontScale, cv::Scalar(200, 200, 250), 3, CV_AA);
				sprintf_s(buf, "%lu%lu",
					ftStop.dwHighDateTime, ftStop.dwLowDateTime);
				cv::putText(frame, buf, cv::Point(30, 60), fontFace, fontScale, cv::Scalar(200, 200, 250), 3, CV_AA);
				sprintf_s(buf, "%10.9g", timestamp);
				cv::putText(frame, buf, cv::Point(30, 90), fontFace, fontScale, cv::Scalar(200, 200, 250), 3, CV_AA);

				++frames;
				if (frames > FRAMES_PER_VIDEO) {
					frames = 0;
					createFile();
				}
				m_video->write(frame);
			}
		}
		m_going = false;
		m_vcap->release();
		m_video->release();
	}

};

#endif// OPEN_CV_RECORDER


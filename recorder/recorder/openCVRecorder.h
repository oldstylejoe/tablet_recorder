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
		//1 is the facing camera on the Surface
		m_vcap.reset(new cv::VideoCapture(0));
		if (!m_vcap->isOpened()) {
			//try to fallback to the default camera
			m_vcap->release();
			m_vcap.reset(new cv::VideoCapture(0));
			if (!m_vcap->isOpened()) {
				throw std::runtime_error("Error opening webcam stream");
			}
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

	//stolen from sourceforge: http://stackoverflow.com/questions/14148758/how-to-capture-the-desktop-in-opencv-ie-turn-a-bitmap-into-a-mat
	//returns the current screen
	void capScreen(cv::Mat& src, const int width, const int height) {

		HDC hwindowDC, hwindowCompatibleDC;

		int srcheight, srcwidth;
		HBITMAP hbwindow;
		BITMAPINFOHEADER  bi;

		//HWND hwnd = GetDesktopWindow();
		hwindowDC = GetDC(NULL);
		hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
		SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

		//RECT windowsize;    // get the height and width of the screen
		//GetClientRect(hwnd, &windowsize);

		srcheight = GetSystemMetrics(SM_CXSCREEN);// windowsize.bottom;
		srcwidth = GetSystemMetrics(SM_CYSCREEN);// windowsize.right;

		//src.create(height, width, CV_8UC4);

		// create a bitmap
		hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
		bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
		bi.biWidth = width;
		bi.biHeight = -height;  //this is the line that makes it draw upside down or not
		bi.biPlanes = 1;
		bi.biBitCount = 32;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		// use the previously created device context with the bitmap
		SelectObject(hwindowCompatibleDC, hbwindow);
		// copy from the window device context to the bitmap device context
		StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
		GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

																										   // avoid memory leak
		DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(NULL, hwindowDC);
	}

	void doVideo() {
		m_going = true;
		int fontFace = cv::FONT_HERSHEY_PLAIN;
		double fontScale = 2;
		int thickness = 3;
		double timestamp;

		FILETIME ftStart, ftStop;
		char buf[1024];

		cv::Mat frame, screen, join;
		int frames = 0;
		while (!m_shutdown) {
			//m_vcap->operator>>(frame);
			if (m_vcap->grab()) {
				GetSystemTimePreciseAsFileTime(&ftStart);
				m_vcap->retrieve(frame);
				GetSystemTimePreciseAsFileTime(&ftStop);
				//screen = frame.clone();
				//capScreen(screen, m_frameWidth, m_frameHeight);

				timestamp = m_vcap->get(CV_CAP_PROP_POS_MSEC);
				sprintf_s(buf, "%lu%lu",
					ftStart.dwHighDateTime, ftStart.dwLowDateTime);
				cv::putText(frame, buf, cv::Point(30, 30), fontFace, fontScale, cv::Scalar(200, 200, 250), 3, CV_AA);
				sprintf_s(buf, "%lu%lu",
					ftStop.dwHighDateTime, ftStop.dwLowDateTime);
				cv::putText(screen, buf, cv::Point(30, 30), fontFace, fontScale, cv::Scalar(200, 200, 250), 3, CV_AA);

				/*try
				{
					cv::vconcat(frame, screen, join);
				}
				catch (cv::Exception & e)
				{
					cout << e.what() << endl;
				}*/
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


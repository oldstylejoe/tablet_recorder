//Joe Snider
//1/16
//
//Consume the keyboard, mouse, and touch events.
//Could add a producer, but the UI thread catches the events and can just
//  write to the threadQueue m_data (the producer).

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>

#include "surface_touch_screen.h"
#include "threadQueue.h"
#include "SafeFile.h"

#ifndef INPUT_CONSUMER
#define INPUT_CONSUMER

using namespace std;

struct SInputHolder {
	int type;
	__int64 timeStamp;
	RAWINPUT* raw;
	shared_ptr<BYTE> lpb;
	UINT dwSize;

	SInputHolder():type(-1),timeStamp(0),raw(NULL) {}//be careful with this constructor, don't deref the RAWINPUT* by calling ToString
	SInputHolder(__int64 timeStamp, HRAWINPUT hraw): type(type), timeStamp(timeStamp) {
		GetRawInputData(hraw, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		lpb.reset(new BYTE[dwSize]);
		if (lpb == NULL)
		{
			OutputDebugString(TEXT("Out of memory in Inputconsumer...fatal flaw ... dying\n"));
			exit(101);
		}

		if (GetRawInputData(hraw, RID_INPUT, lpb.get(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
			OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

		raw = (RAWINPUT*)lpb.get();
		type = raw->header.dwType;

	}

	~SInputHolder() {
	}

	string ToString() const {
		stringstream ss;
		ss << timeStamp << " ";
		if (type == RIM_TYPEMOUSE) {
			ss << "mouse "
				<< raw->data.mouse.lLastX << " "
				<< raw->data.mouse.lLastY << " "
				<< raw->data.mouse.ulButtons << " "
				<< raw->data.mouse.ulExtraInformation << " "
				<< raw->data.mouse.ulRawButtons << " "
				<< raw->data.mouse.usButtonData << " "
				<< raw->data.mouse.usButtonFlags << " "
				<< raw->data.mouse.usFlags;
		}
		else if (type == RIM_TYPEKEYBOARD) {
			ss << "keyboard "
				<< raw->data.keyboard.ExtraInformation << " "
				<< raw->data.keyboard.Flags << " "
				<< raw->data.keyboard.MakeCode << " "
				<< raw->data.keyboard.Message << " "
				<< raw->data.keyboard.Reserved << " "
				<< raw->data.keyboard.VKey;
		}
		else if (type == RIM_TYPEHID) {
			for (DWORD index(0); index < raw->data.hid.dwCount; ++index) {
				SurfaceTouchScreen::TD* td = ((SurfaceTouchScreen::TD*)&raw->data.hid.bRawData[raw->data.hid.dwSizeHid * index]);
				ss << "touch " << (int)td->status1 << " " << td->GetDuration() << " "
					<< (int)td->touchType << " "
					<< (int)td->touchNumber << " "
					<< (int)td->status2 << " "
					<< td->X() << " "
					<< td->Y() << (index+1 == raw->data.hid.dwCount)?"":"\n";
			}
		}

		return ss.str();
	}
};

class CInputConsumer{
	CSafeArbFile<SInputHolder> m_file;
	shared_ptr<thread> m_workerThread;
	atomic_bool m_going;

public:
	threadQueue<SInputHolder> m_data;

	CInputConsumer(string file = "c:\\DataLogs\\input\\test_input") : m_file(file) {
		m_going = false;
	}
	~CInputConsumer() {
		m_going = false;
		if (m_workerThread && m_workerThread->joinable()) {
			m_workerThread->join();
		}
	}

	void startConsuming() {
		m_going = true;
		m_workerThread.reset(new thread(&CInputConsumer::run, this));
	}

	bool isGoing() { return m_going; }

	void stopConsuming() {
		m_going = false;
		if (m_workerThread && m_workerThread->joinable()) {
			m_workerThread->join();
		}
		m_file.flush();
	}

private:
	void run() {
		chrono::milliseconds waitTime(10);
		while (m_going) {
			SInputHolder val;
			while (m_data.pop(val)) {
				m_file.write(val);
			}
			this_thread::sleep_for(waitTime);
		}
	}
};

#endif// INPUT_CONSUMER


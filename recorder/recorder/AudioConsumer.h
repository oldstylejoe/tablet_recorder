//Joe Snider
//4/15
//
//Consume the audio data (just save it to a file)

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>

#include "AudioProducer.h"
#include "threadQueue.h"
#include "SafeFile.h"

#ifndef AUDIO_CONSUMER
#define AUDIO_CONSUMER

using namespace std;

class CAudioConsumer{
	CSafeFile m_file;
	shared_ptr<CAudioProducer> m_pd;
	shared_ptr<thread> m_workerThread;
	atomic_bool m_going;

public:
	CAudioConsumer(string file= "c:\\DataLogs\\audio\\test_audio") : m_file(file) {
		m_going = false;
	}
	~CAudioConsumer() {
		m_going = false;
		if (m_workerThread && m_workerThread->joinable()) {
			m_workerThread->join();
		}
	}

	void startConsuming(shared_ptr<CAudioProducer> pd) {
		m_pd = pd;
		m_going = true;
		m_workerThread.reset(new thread(&CAudioConsumer::run, this));
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
			vector<float> val;
			while(true) {
				val.clear();
				m_pd->m_data.pop(val);
				if (val.size() > 0) {
					m_file.write(val);
				}
				else {
					break;
				}
			}
			this_thread::sleep_for(waitTime);
		}
	}

};

#endif// AUDIO_CONSUMER


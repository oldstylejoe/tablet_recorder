//Joe Snider
//4/15
//
//Safely open a file and close it every hour (or whatever) while maintaining accurate streaming.
//Whatever gets sent in must be streamable into a stringstream.
//This is generally meant for text type files.

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <memory>
#include <deque>
#include <atomic>
#include <mutex>

#include "snappy.h"

#include <filesystem>

#ifndef SAFE_FILE
#define SAFE_FILE

using namespace std;

const int WRITE_SIZE = 10; //buffer this many samples before writing

//create a filename with the given base name and extension
//   <base>_yyyy.mm.dd.HH.<extension>
//creates at most one file per hour (system time)
//logs that go faster than that should be careful to append
//the return value specifies if the file exists already and what number would have to be
//  appended to make a new file (i.e. <base>_yyyy.mm.dd.HH_<ret>.<extension> can be created)
//        greater than zero indicates the file already exists
//        a negative number means more than 1000 such files exist
//uses <filesystem> from tr2
//
//This is meant as a generally useful utility, so it is just a global function
namespace SafeFile {
	static int CreateSafeTimeFileNameNumbered(string base, string extension, string& fileName) {
		auto ttime_t = chrono::system_clock::to_time_t(chrono::system_clock::now());
		tm ttm;
		localtime_s(&ttm, &ttime_t);

		char date_time_format[] = "%Y.%m.%d.%H";
		char time_str[] = "yyyy.mm.dd.HHaaaaaaaa"; //not sure why it needs the extra space, minor problem
		strftime(time_str, strlen(time_str), date_time_format, &ttm);

		//add in the subject id if its available
		string subjid;
		ifstream subjFile("c:\\DataLogs\\subject_id.txt");
		if (subjFile.good()) {
			while (subjFile >> subjid);
		}
		subjFile.close();
		base += "_";
		base += subjid;

		for (int i = 0; i < 1000; ++i) {
			fileName = base + "_" + time_str + "_" + to_string(i) + "." + extension;
			if (!tr2::sys::exists(fileName)) {
				return i;
			}
		}
		//failed to find an appendable file
		return -1;
	}

	static int CreateSafeTimeFileName(string base, string extension, string& fileName) {
		auto ttime_t = chrono::system_clock::to_time_t(chrono::system_clock::now());
		tm ttm;
		localtime_s(&ttm, &ttime_t);

		char date_time_format[] = "%Y.%m.%d.%H";
		char time_str[] = "yyyy.mm.dd.HHaaaaaaaa"; //not sure why it needs the extra space, minor problem
		strftime(time_str, strlen(time_str), date_time_format, &ttm);

		//add in the subject id if its available
		string subjid;
		ifstream subjFile("c:\\DataLogs\\subject_id.txt");
		if (subjFile.good()) {
			while (subjFile >> subjid);
		}
		subjFile.close();
		base += "_";
		base += subjid;

		fileName = base + "_" + time_str + "." + extension;
		return 0;
	}
}

class CSafeFile{
	vector<vector<float> > m_buf;
	vector<vector<float> > m_writeBuf;
	atomic_bool m_workerBusy;
	shared_ptr<thread> m_worker;

	mutex m_Blocker;

public:
	string m_base;
	string m_name;

	CSafeFile(const string& inBase) : m_base(inBase) {
		m_workerBusy = false;
	}
	~CSafeFile() {
		if (m_worker && m_worker->joinable()) {
			m_worker->join();
		}
		startSave();
		if (m_worker && m_worker->joinable()) {
			m_worker->join();
		}
	}

public:
	void write(const vector<float>& x) {
		m_buf.push_back(x);
		if (shouldSave()) {
			startSave();
		}
	}

	//read the queue
	void flush() {
		if (m_worker && m_worker->joinable()) {
			m_worker->join();
		}
		startSave();
	}

private:

	//startup the thread to read the queue and write the file
	void startSave() {
		SafeFile::CreateSafeTimeFileName(m_base, "bxt", m_name);
		//this makes an extra copy (I think)
		unique_lock<mutex> lock(m_Blocker);
		m_writeBuf.insert(m_writeBuf.end(), m_buf.begin(), m_buf.end());
		m_buf.clear();
		lock.unlock();
		if (!m_workerBusy) {
			if (m_worker && m_worker->joinable()) {
				m_worker->join();
			}
			m_worker.reset(new thread(&CSafeFile::doSave, this));
		}
	}

	//enough in the buffer to be worth it and worker is free
	//this is tweakable
	bool shouldSave() {
		return !m_workerBusy && m_buf.size() > WRITE_SIZE;
	}

	void doSave() {
		m_workerBusy = true;
		//TODO make this into a compressed file type
		//using snappy (7/15). output format is a binary unsigned long that contains the length of the compressed
		//   snappy sequence coming up, n. Then, the compressed snappy sequence. Read n bytes and decompress to use.
		ofstream fil(m_name.c_str(), ofstream::out | ofstream::app | ios::binary);
		//ofstream ftest("test_safe_file.txt", ofstream::app);
		if (fil.good()) {
			unique_lock<mutex> lock(m_Blocker);
			for (auto s : m_writeBuf) {
				unsigned long n = s.size();
				char* ss = reinterpret_cast<char*>(s.data());
				int charLength = s.size()*sizeof(float) / sizeof(char);
				string compressed;
				int compressedLength = snappy::Compress(ss, charLength, &compressed);
				fil.write(reinterpret_cast<char*>(&compressedLength), sizeof(unsigned long));
				fil.write(compressed.data(), compressedLength);

				//ftest << n << " " << compressedLength << "\n";
			}
			m_writeBuf.clear();
			lock.unlock();
			//fil.close(); //this crashes it for some reason?

		}
		else {
			//TODO: throw an error here
		}
		m_workerBusy = false;
	}


};


//This class does the same as safefile, but can handle arbitrary output types.
//This has no compression.
//Use it for data that comes in relatively slowly (like touches).
//type T must specify a ToString function.
template<class T>
class CSafeArbFile {
	vector<T> m_buf;
	vector<T> m_writeBuf;
	atomic_bool m_workerBusy;
	shared_ptr<thread> m_worker;

	mutex m_Blocker;

public:
	string m_base;
	string m_name;

	CSafeArbFile(const string& inBase) : m_base(inBase) {
		m_workerBusy = false;
	}
	~CSafeArbFile() {
		if (m_worker && m_worker->joinable()) {
			m_worker->join();
		}
		startSave();
		if (m_worker && m_worker->joinable()) {
			m_worker->join();
		}
	}

public:
	void write(const T& x) {
		m_buf.push_back(x);
		if (shouldSave()) {
			startSave();
		}
	}

	//read the queue
	void flush() {
		if (m_worker && m_worker->joinable()) {
			m_worker->join();
		}
		startSave();
	}

private:

	//startup the thread to read the queue and write the file
	void startSave() {
		SafeFile::CreateSafeTimeFileName(m_base, "txt", m_name);
		//this makes an extra copy (I think)
		//the unique_lock is so I can delete m_buf
		unique_lock<mutex> lock(m_Blocker);
		m_writeBuf.insert(m_writeBuf.end(), m_buf.begin(), m_buf.end());
		m_buf.clear();
		lock.unlock();
		if (!m_workerBusy) {
			if (m_worker && m_worker->joinable()) {
				m_worker->join();
			}
			m_worker.reset(new thread(&CSafeArbFile::doSave, this));
		}
	}

	//enough in the buffer to be worth it and worker is free
	//this is tweakable
	bool shouldSave() {
		return !m_workerBusy && m_buf.size() > WRITE_SIZE;
	}

	void doSave() {
		m_workerBusy = true;
		//TODO make this into a compressed file type
		//using snappy (7/15). output format is a binary unsigned long that contains the length of the compressed
		//   snappy sequence coming up, n. Then, the compressed snappy sequence. Read n bytes and decompress to use.
		ofstream fil(m_name.c_str(), ofstream::app);
		//ofstream ftest("test_safe_file.txt", ofstream::app);
		if (fil.good()) {
			unique_lock<mutex> lock(m_Blocker);
			for (auto s : m_writeBuf) {
				fil << s.ToString() << "\n";
			}
			m_writeBuf.clear();
			lock.unlock();

		}
		else {
			//TODO: throw an error here
		}
		m_workerBusy = false;
	}

};

#endif// SAFE_FILE

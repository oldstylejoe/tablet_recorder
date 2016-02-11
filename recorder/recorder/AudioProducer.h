//Joe Snider
//4/15
//
//Read from an audio stream.
//Uses the WASAPI for fastest possible access.
//The heart of it was stolen from labstreaminglayer api
//

#include <Windows.h>
#include <thread>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <vector>
#include <string>
#include <atomic>
#include <fstream>

#include <Audioclient.h>
#include <Mmdeviceapi.h>

#include "threadQueue.h"

#ifndef AUDIO_PRODUCER
#define AUDIO_PRODUCER

using namespace std;

class CAudioProducer{
	shared_ptr<thread> m_readerThread;
	atomic_bool m_shutdown;
	atomic_bool m_going;
	atomic_ulong m_frameSize;

	atomic_bool m_loopback;

	// desired buffer duration in seconds
	const float buffer_duration = 1.0f;
	// update interval in seconds
	const float update_interval = 0.05f;
	// retry interval after device failure
	const float retry_interval = 0.25;
	// time conversion factor
	const int REFTIMES_PER_SEC = 10000000;

public:
	threadQueue<vector<float> > m_data;

	CAudioProducer(const bool loopback=false):m_loopback(loopback) {
		m_going = false;
		m_shutdown = false;
		m_frameSize = 0;
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(hr)) {
			string t = "Could not initialize COM.";
			t += "\n";
			t += std::to_string(hr);
			wchar_t buf[1024];
			size_t tt;
			mbstowcs_s(&tt, buf, 1024, t.c_str(), 1024);
			MessageBox(NULL, buf, _T("Error"), MB_ICONERROR | MB_OK);
		}
		initAudio();
	}
	~CAudioProducer() {
		m_shutdown = true;
		if (m_readerThread && m_readerThread->joinable()) {
			m_readerThread->join();
		}
	}

public:
	//might hang the thread
	bool isStarted() const { return !m_shutdown; }
	bool isGoing() const { return m_going; }

	void pause() { m_going = false; }
	void unpause() { m_going = true; }

	void initAudio() {
		if (m_readerThread) {
			// === perform unlink action ===
			m_shutdown = true;

			try {
				m_readerThread->join();
				m_readerThread.reset();
			}
			catch (std::exception &e) {
				MessageBox(NULL, _T("Error stopping the Audio thread. This is serious."), _T("Error"), MB_ICONERROR | MB_OK);
				//QMessageBox::critical(this, "Error", (std::string("Could not stop the background processing: ") += e.what()).c_str(), QMessageBox::Ok);
				return;
			}
		}
		else {
			// === perform link action ===
			m_shutdown = false;

			IMMDeviceEnumerator *pEnumerator = NULL;
			IMMDevice *pDevice = NULL;
			IAudioClient *pAudioClient = NULL;
			IAudioCaptureClient *pCaptureClient = NULL;
			WAVEFORMATEX *pwfx = NULL;
			LPWSTR deviceId = L"unidentified";
			HRESULT hr;

			try {
				// get the device enumerator
				if (FAILED(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator))) {
					string t = "Could not create multimedia device enumerator.";
					t += "\n";
					t += std::to_string(hr);
					throw std::runtime_error(t.c_str());
				}

				// ask for the default capture endpoint (note: this is acting as a "multimedia" client)
				//
				if (m_loopback) {
					if (FAILED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice)))
						throw std::runtime_error("Could not get a loopback recording device.");
				}
				else {
					if (FAILED(pEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &pDevice)))
						throw std::runtime_error("Could not get a default recording device -- is one selected?");
				}

				// try to activate and IAudioClient
				if (FAILED(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient))) {
					DWORD state;
					pDevice->GetState(&state);
					switch (state) {
					case DEVICE_STATE_DISABLED:
						throw std::runtime_error("Could not activate the default recording device -- is it enabled?.");
					case DEVICE_STATE_UNPLUGGED:
						throw std::runtime_error("Could not activate the default recording device -- is it plugged in?");
					case DEVICE_STATE_NOTPRESENT:
						throw std::runtime_error("Could not activate the default recording device -- is one selected?");
					default:
						throw std::runtime_error("Failed to activate the default recording device.");
					}
				}

				// get device ID
				pDevice->GetId(&deviceId);

				// get mixing format
				if (FAILED(pAudioClient->GetMixFormat(&pwfx)))
					throw std::runtime_error("Could not get the data format of the recording device -- make sure it is available for use.");
				//this causes the data object read from the audio to be floats (multiplexed from l/r audio)
				pwfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
				pwfx->wBitsPerSample = 32;
				pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
				pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
				pwfx->cbSize = 0;

				// initialize a shared buffer
				if (m_loopback) {
					if (FAILED(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC*buffer_duration, 0, pwfx, NULL)))
						throw std::runtime_error("Could not initialize a loopback audio client.");
				}
				else {
					if (FAILED(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, REFTIMES_PER_SEC*buffer_duration, 0, pwfx, NULL)))
						throw std::runtime_error("Could not initialize a shared audio client.");
				}

				// get the capture service
				if (FAILED(pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient)))
					throw std::runtime_error("Unexpected error: could not get the audio capture service.");

				//find the frame size
				WAVEFORMATEX * pMixFormat;
				if(FAILED(pAudioClient->GetMixFormat(&pMixFormat)))
					throw std::runtime_error("Unexpected error: could not get the audio capture format.");
				m_frameSize = (pMixFormat->wBitsPerSample / 8) * pMixFormat->nChannels;

				// try to start the actual recording
				if (FAILED(pAudioClient->Start()))
					throw std::runtime_error("Could not start recording.");

				// start the reader thread (which is responsible for deallocation of everything)
				m_readerThread.reset(new thread(&CAudioProducer::processSamples, this, pEnumerator, pDevice, pAudioClient, pCaptureClient, pwfx, deviceId));

				// now we are linked
				//ui->linkButton->setText("Unlink");

			}
			catch (const std::runtime_error &e) {
				// got an exception during initialization: release resources...
				if (pCaptureClient)
					pCaptureClient->Release();
				if (pAudioClient)
					pAudioClient->Release();
				if (pDevice)
					pDevice->Release();
				if (pEnumerator)
					pEnumerator->Release();
				if (pwfx)
					CoTaskMemFree(pwfx);
				// and show message box
				//AfxMessageBox(e.what());
				wchar_t buf[1024];
				size_t t;
				mbstowcs_s(&t, buf, 1024, e.what(), 1024);
				MessageBox(NULL, buf, _T("Error"), MB_ICONERROR | MB_OK);
				//QMessageBox::critical(this, "Initialization Error", (std::string("Could not start recording: ") += e.what()).c_str(), QMessageBox::Ok);
			}
		}

	}

private:
	void processSamples(IMMDeviceEnumerator *pEnumerator, IMMDevice *pDevice, IAudioClient *pAudioClient, IAudioCaptureClient *pCaptureClient, WAVEFORMATEX *pwfx, LPWSTR devID) {
		// repeat until interrupted (can re-connect if device was lost)
		chrono::seconds oneSec(1);
		chrono::milliseconds oneUI(int(1000.0f*update_interval + 0.5f));
		chrono::milliseconds oneRI(int(1000.0f*retry_interval + 0.5));


		while (!m_shutdown) {
			try {
				try {
					// for each chunk update...
					while (!m_shutdown) {
						// sleep for the update interval
						this_thread::sleep_for(oneUI);

						// get the length of the first packet
						UINT32 packetLength = 0;
						if (FAILED(pCaptureClient->GetNextPacketSize(&packetLength)))
							throw std::runtime_error("Device got invalidated.");

						LARGE_INTEGER now, freq;				// current time, timer frequency
						QueryPerformanceFrequency(&freq);
						FILETIME ftNow;
						SYSTEMTIME stNow;

						int channels = pwfx->nChannels;

						// consume each chunk...
						while (packetLength) {
							BYTE *pData;					// pointer to the data...
							DWORD flags;					// flags of the recording device
							UINT64 captureTime;				// capture time of the first sample (in 100-ns units)
							UINT32 numFramesAvailable;		// number of frames available in the buffer

							// get the available data in the shared buffer
							if (FAILED(pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, &captureTime)))
								throw std::runtime_error("Device got invalidated.");

							if (m_going) {
								// determine the age of the last sample of the chunk
								GetSystemTimePreciseAsFileTime(&ftNow);
								QueryPerformanceCounter(&now);
								//FileTimeToSystemTime(&ftNow, &stNow);

								double firstSampleAge = (double)(now.QuadPart) / freq.QuadPart - (double)(captureTime) / REFTIMES_PER_SEC;	// time passed since the sample was captured
								//double lastSampleAge = (firstSampleAge - ((double)(numFramesAvailable - 1)) / pwfx->nSamplesPerSec);		// the last sample is younger than the first one...
								//double lastSampleTime = (double)(now.QuadPart) / freq.QuadPart - lastSampleAge;

								if (!(flags&AUDCLNT_BUFFERFLAGS_SILENT)) {
									vector<float> header(4);
									header[0] = ftNow.dwHighDateTime;
									header[1] = ftNow.dwLowDateTime;
									header[2] = firstSampleAge;
									header[3] = pwfx->nSamplesPerSec;
									m_data.push(header);
									//float *audioValues = (float*)pData;
									//for (unsigned k = 0; k < numFramesAvailable; k++)
									//	chunk[k].assign(&audioValues[k*channels], &audioValues[(k + 1)*channels]);
									//m_data.push(chunk);

									vector<float> audio(channels*numFramesAvailable);
									float* audioF = reinterpret_cast<float*>(pData);
									for (int i = 0; i < channels*numFramesAvailable; ++i) {
										audio[i] = audioF[i];
									}
									m_data.push(audio);
								}
							}

							// move on to next chunk
							if (FAILED(pCaptureClient->ReleaseBuffer(numFramesAvailable)))
								throw std::runtime_error("Device got invalidated.");
							if (FAILED(pCaptureClient->GetNextPacketSize(&packetLength)))
								throw std::runtime_error("Device got invalidated.");
						}
					}
				}
				catch (std::runtime_error &) {
					// device invalidated -- release...
					pCaptureClient->Release();
					pCaptureClient = 0;
					pAudioClient->Release();
					pAudioClient = 0;
					while (true) {
						// sleep for a bit
						this_thread::sleep_for(oneRI);
						// try to reacquire with same parameters
						if (FAILED(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient)))
							continue;
						if (FAILED(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, REFTIMES_PER_SEC*buffer_duration, 0, pwfx, NULL)))
							continue;
						if (FAILED(pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient)))
							continue;
						if (FAILED(pAudioClient->Start()))
							continue;
						// now back in business again...
						break;
					}
				}
			}
			catch (std::runtime_error &) {
				// thread interrupted -- exit!
				break;
			}
		}
	}
};

#endif// AUDIO_PRODUCER

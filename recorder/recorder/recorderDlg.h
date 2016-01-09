
// MFCApplication1Dlg.h : header file
//

#pragma once

#include <memory>

#include "AudioProducer.h"
#include "AudioConsumer.h"
#include "openCVRecorder.h"
#include "surface_touch_screen.h"

// CMFCApplication1Dlg dialog
class CMFCApplication1Dlg : public CDialogEx
{
	std::shared_ptr<CAudioProducer> m_audioProducer;
	std::shared_ptr<CAudioConsumer> m_audioConsumer;
	std::shared_ptr<CAudioProducer> m_loopbackProducer;
	std::shared_ptr<CAudioConsumer> m_loopbackConsumer;
	std::shared_ptr<COpenCVRecorder> m_video;
	CStatic m_labelAudio;

// Construction
public:
	CMFCApplication1Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MFCAPPLICATION1_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonAudio();
	afx_msg void OnBnClickedButtonWebcam();
	afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData);
	afx_msg void OnBnClickedButtonLoopback();
	afx_msg void OnRawInput(UINT nInputcode, HRAWINPUT hRawInput);

	void registerDevices() {
		RAWINPUTDEVICE Rid[3];

		Rid[0].usUsagePage = 0x01;
		Rid[0].usUsage = 0x02;
		Rid[0].dwFlags = RIDEV_INPUTSINK;   // adds HID mouse and also ignores legacy mouse messages
		Rid[0].hwndTarget = m_hWnd;

		Rid[1].usUsagePage = 0x01;
		Rid[1].usUsage = 0x06;
		Rid[1].dwFlags = RIDEV_INPUTSINK;   // adds HID keyboard and also ignores legacy keyboard messages
		Rid[1].hwndTarget = m_hWnd;

		Rid[1].usUsagePage = 13;
		Rid[1].usUsage = 4;
		Rid[1].dwFlags = RIDEV_INPUTSINK;   // adds surface touch screen
		Rid[1].hwndTarget = m_hWnd;

		if (RegisterRawInputDevices(Rid, 3, sizeof(Rid[0])) == FALSE)
		{
			DWORD err = GetLastError();
		}
	}
};

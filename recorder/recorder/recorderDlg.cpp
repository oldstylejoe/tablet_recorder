
// MFCApplication1Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "recorder.h"
#include "recorderDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCApplication1Dlg dialog



CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCApplication1Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_AUDIO, &CMFCApplication1Dlg::OnBnClickedButtonAudio)
	ON_BN_CLICKED(IDC_BUTTON_WEBCAM, &CMFCApplication1Dlg::OnBnClickedButtonWebcam)
	ON_WM_POWERBROADCAST()
	ON_BN_CLICKED(IDC_BUTTON_LOOPBACK, &CMFCApplication1Dlg::OnBnClickedButtonLoopback)
END_MESSAGE_MAP()


// CMFCApplication1Dlg message handlers

BOOL CMFCApplication1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_audioProducer.reset(new CAudioProducer);
	m_audioConsumer.reset(new CAudioConsumer);
	m_loopbackProducer.reset(new CAudioProducer(true));
	m_loopbackConsumer.reset(new CAudioConsumer("c:\\DataLogs\\audio\\test_loopback"));
	m_video.reset(new COpenCVRecorder);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFCApplication1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCApplication1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFCApplication1Dlg::OnBnClickedButtonAudio()
{
	// TODO: Add your control notification handler code here
	CStatic* lab = (CStatic*)GetDlgItem(IDC_STATIC_AUDIO);
	if (m_audioProducer->isStarted() && m_audioProducer->isGoing() && m_audioConsumer->isGoing()) {
		lab->SetWindowTextW(_T("Stopping"));
		m_audioProducer->pause();
		m_audioConsumer->stopConsuming();
		lab->SetWindowTextW(_T("Stopped"));
	}
	else {
		lab->SetWindowTextW(_T("Starting"));
		m_audioConsumer->startConsuming(m_audioProducer);
		m_audioProducer->unpause();
		lab->SetWindowTextW(_T("On"));
	}
}


void CMFCApplication1Dlg::OnBnClickedButtonLoopback()
{
	// TODO: Add your control notification handler code here
	CStatic* lab = (CStatic*)GetDlgItem(IDC_STATIC_LOOPBACK);
	if (m_loopbackProducer->isStarted() && m_loopbackProducer->isGoing() && m_loopbackConsumer->isGoing()) {
		lab->SetWindowTextW(_T("Stopping"));
		m_loopbackProducer->pause();
		m_loopbackConsumer->stopConsuming();
		lab->SetWindowTextW(_T("Stopped"));
	}
	else {
		lab->SetWindowTextW(_T("Starting"));
		m_loopbackConsumer->startConsuming(m_loopbackProducer);
		m_loopbackProducer->unpause();
		lab->SetWindowTextW(_T("On"));
	}
}


void CMFCApplication1Dlg::OnBnClickedButtonWebcam()
{
	// TODO: Add your control notification handler code here
	CStatic* lab = (CStatic*)GetDlgItem(IDC_STATIC_WEBCAM);
	if (!m_video->isGoing()) {
		lab->SetWindowTextW(_T("Starting"));
		m_video->startVideo();
		if (m_video->isGoing()) {
			lab->SetWindowTextW(_T("Started"));
		}
		else {
			//give it 100ms
			Sleep(100);
			if (m_video->isGoing()) {
				lab->SetWindowTextW(_T("Started"));
			}
			else {
				lab->SetWindowTextW(_T("Failed"));
			}
		}
	}
	else {
		lab->SetWindowTextW(_T("Stopping"));
		m_video->stopVideo();
		lab->SetWindowTextW(_T("Stopped"));
	}
}


UINT CMFCApplication1Dlg::OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData)
{
	if (nPowerEvent == PBT_APMSUSPEND) {
		CStatic* lab = (CStatic*)GetDlgItem(IDC_STATIC_WEBCAM);
		if (m_video->isGoing()) {
			lab->SetWindowTextW(_T("Stopping"));
			m_video->stopVideo();
			lab->SetWindowTextW(_T("Stopped"));
		}

		// TODO: Add your control notification handler code here
		lab = (CStatic*)GetDlgItem(IDC_STATIC_AUDIO);
		if (m_audioProducer->isStarted() && m_audioProducer->isGoing() && m_audioConsumer->isGoing()) {
			lab->SetWindowTextW(_T("Stopping"));
			m_audioProducer->pause();
			m_audioConsumer->stopConsuming();
			lab->SetWindowTextW(_T("Stopped"));
		}

	}
	return CDialogEx::OnPowerBroadcast(nPowerEvent, nEventData);
}

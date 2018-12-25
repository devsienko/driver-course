
// CpuMonitorMfcDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CpuMonitorMfc.h"
#include "CpuMonitorMfcDlg.h"
#include "afxdialogex.h"
#include "winioctl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define DEVICE_REC CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCpuMonitorMfcDlg dialog



CCpuMonitorMfcDlg::CCpuMonitorMfcDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CPUMONITORMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCpuMonitorMfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCpuMonitorMfcDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CCpuMonitorMfcDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CCpuMonitorMfcDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CCpuMonitorMfcDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CCpuMonitorMfcDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON1, &CCpuMonitorMfcDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CCpuMonitorMfcDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON4, &CCpuMonitorMfcDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON3, &CCpuMonitorMfcDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON5, &CCpuMonitorMfcDlg::OnBnClickedButton5)
END_MESSAGE_MAP()


// CCpuMonitorMfcDlg message handlers

BOOL CCpuMonitorMfcDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCpuMonitorMfcDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCpuMonitorMfcDlg::OnPaint()
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
HCURSOR CCpuMonitorMfcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



HANDLE devicehandle = NULL;

void CCpuMonitorMfcDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	devicehandle = CreateFile(L"\\\\.\\mydevicelink123", GENERIC_ALL, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
	if (devicehandle == INVALID_HANDLE_VALUE) {
		MessageBox(L"not valid value", 0, 0);
		return;
	}
	//to do your thing
	MessageBox(L"valid value", 0, 0);
}

void CCpuMonitorMfcDlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	if (devicehandle != INVALID_HANDLE_VALUE)
		CloseHandle(devicehandle);
}

void CCpuMonitorMfcDlg::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	WCHAR* message = L"send sample from mfc";
	ULONG returnLength = 0;
	char wr[4] = { 0 };
	if (devicehandle != INVALID_HANDLE_VALUE && devicehandle != NULL) {
		if (!DeviceIoControl(devicehandle, DEVICE_SEND, message, (wcslen(message) + 1) * 2, NULL,
			0, &returnLength, 0))
		{
			MessageBox(L"DeviceIoControl error", 0, 0);
		}
		else {
			_itoa_s(returnLength, wr, 10);
			MessageBoxA(0, wr, 0, 0);
		}
	}
}

void CCpuMonitorMfcDlg::OnBnClickedButton4()
{
	// TODO: Add your control notification handler code here
	WCHAR message[1024] = { 0 };
	ULONG returnLength = 0;

	if (devicehandle != INVALID_HANDLE_VALUE && devicehandle != NULL) {
		if (!DeviceIoControl(devicehandle, DEVICE_REC, NULL, 0, message, 1024, &returnLength, 0))
		{
			MessageBox(L"DeviceIoControl error", 0, 0);
		}
		else {
			MessageBox(message, 0, 0);
		}
	}
}

typedef struct {
	ULONG cpuid;
	ULONG tempdata;
} CPUTEMP, *PCPUTEMP;

UINT tempread(LPVOID lpParam) {
	ULONG returnLength;
	CString output;
	HWND ec;
	CPUTEMP dataarray[4] = { 0 };
	ec = ::GetDlgItem(AfxGetMainWnd()->m_hWnd, IDC_EDIT1);
	while (1) {

		Sleep(1000);
		if (DeviceIoControl(devicehandle, DEVICE_REC, NULL, 0, &dataarray, sizeof(CPUTEMP) * 4, &returnLength, 0)) {
			output.Format(_T("CPU %d: %d C\r\nCPU %d: %d C\r\nCPU %d: %d C\r\nCPU %d: %d C\r\n"),
				dataarray[0].cpuid, dataarray[0].tempdata,
				dataarray[1].cpuid, dataarray[1].tempdata,
				dataarray[2].cpuid, dataarray[2].tempdata,
				dataarray[3].cpuid, dataarray[3].tempdata);

			::SetWindowText(ec, output);
		}
	}
}

void CCpuMonitorMfcDlg::OnBnClickedButton5()
{
	CHAR* message = "start";
	ULONG returnlength;
	if (devicehandle != INVALID_HANDLE_VALUE && devicehandle != NULL) {

		if (!DeviceIoControl(devicehandle, DEVICE_SEND, message, 6, NULL, 0, &returnlength, 0)) {
			MessageBox(L"DeviceIoControl error", 0, 0);
			return;
		}

		AfxBeginThread(
			tempread,
			NULL,
			THREAD_PRIORITY_NORMAL,
			0,
			0,
			NULL
		);
	}
}

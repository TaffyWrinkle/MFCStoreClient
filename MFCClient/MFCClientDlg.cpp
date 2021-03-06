// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "stdafx.h"
#include "MFCClient.h"
#include "MFCClientDlg.h"
#include "afxdialogex.h"
#include "WindowsVersionHelper.h"

#include <string>
#include <sstream>     

using namespace WinRT;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
    //Windows::Foundation::Initialize();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCClientDlg dialog
CMFCClientDlg::CMFCClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCCLIENT_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMFCClientDlg::~CMFCClientDlg()
{
    // release the WindowsStore object
    if (m_windowsStoreFreeFunc)
    {
        m_windowsStoreFreeFunc(m_storePtr);
    }

    //Free the DLL
    if (m_dllHandle)
    {
        FreeLibrary(m_dllHandle);
    }
}

void CMFCClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMFCClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_CHECK_LICENSE, &CMFCClientDlg::OnBnClickedCheckLicense)
    ON_BN_CLICKED(IDC_BUY, &CMFCClientDlg::OnBnClickedBuy)
    ON_BN_CLICKED(IDC_LAUNCH_WIN32_APP, &CMFCClientDlg::OnLaunchWin32App)
END_MESSAGE_MAP()

// CMFCClientDlg message handlers

BOOL CMFCClientDlg::OnInitDialog()
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
    // initial the StoreContext

    WindowsStoreErrorType result = WINRT_NO_ERROR;

    //Load the WindowsStore dll
    if (windows10orGreaterWithManifest())
    {
        m_dllHandle = LoadLibrary(L"WindowsStoreDLL.dll");
    }

    if (NULL == m_dllHandle)
    {
        SetDlgItemText(IDC_TEXT_DISPLAY, L"Unable to load WindowsStoreDLL.dll");
        return TRUE;
    }

    // Get WindowsStore DLL function pointers. Error checking needs to be added!
    m_windowsStoreInitFunc = reinterpret_cast<WindowsStoreInitializeFunc>(::GetProcAddress(m_dllHandle, "store_initialize"));
    m_windowsStorePurchaseFunc = reinterpret_cast<WindowsStorePurchaseFunc>(::GetProcAddress(m_dllHandle, "store_purchase"));
    m_windowsStoreLicenseStateFunc = reinterpret_cast<WindowsStoreLicenseStateFunc>(::GetProcAddress(m_dllHandle, "store_license_state"));
    m_windowsStoreGetPriceFunc = reinterpret_cast<WindowsStoreGetPriceFunc>(::GetProcAddress(m_dllHandle, "store_get_price"));
    m_windowsStoreFreeFunc = reinterpret_cast<WindowsStoreFreeFunc>(::GetProcAddress(m_dllHandle, "store_free"));

    CWnd* pWnd = GetDlgItem(IDC_TEXT_DISPLAY);

    // initialize Windows Store functionality
    result = m_windowsStoreInitFunc(&m_storePtr, pWnd->GetSafeHwnd(), &CMFCClientDlg::StoreLicenseStateChangedCallback, (void*)this);
    if (result != WINRT_NO_ERROR)
    {
        switch (result)
        {
        case WINRT_WINDOWS_RUNTIME_ERROR:
            SetDlgItemText(IDC_TEXT_DISPLAY, L"Unable to initialize Windows Runtime");
            break;

        case WINRT_WINDOWS_VERSION_ERROR:
            SetDlgItemText(IDC_TEXT_DISPLAY, L"This version of Windows does not support Windows::Services::Store");
            break;

        case WINRT_APP_PACKAGE_ERROR:
            SetDlgItemText(IDC_TEXT_DISPLAY, L"This app is not running inside of an App Package");
            break;

        default:
            SetDlgItemText(IDC_TEXT_DISPLAY, L"Unable to initialize Windows Store");
            break;
        }
    }

    m_windowsStoreGetPriceFunc(m_storePtr, &CMFCClientDlg::StoreGetPriceCallback, (void*)this);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFCClientDlg::OnBnClickedCheckLicense()
{
    m_windowsStoreLicenseStateFunc(m_storePtr, &CMFCClientDlg::StoreLicenseStateCallback, (void*)this);
}

void CMFCClientDlg::OnBnClickedBuy()
{
    m_windowsStorePurchaseFunc(m_storePtr, &CMFCClientDlg::StorePurchaseCallback, (void*)this);
}



void CMFCClientDlg::StorePurchaseCallback(int error, const wchar_t* message, void* userData)
{
    CMFCClientDlg* instance = (CMFCClientDlg*)userData;
    instance->SetDlgItemText(IDC_TEXT_DISPLAY, message);
}

void CMFCClientDlg::StoreLicenseStateChangedCallback(int error, const wchar_t* message, void* userData)
{
    CMFCClientDlg* instance = (CMFCClientDlg*)userData;
    instance->SetDlgItemText(IDC_TEXT_DISPLAY, message);
}

void CMFCClientDlg::StoreLicenseStateCallback(int error, const wchar_t* message, void* userData)
{
    CMFCClientDlg* instance = (CMFCClientDlg*)userData;
    instance->SetDlgItemText(IDC_TEXT_DISPLAY, message);
}

void CMFCClientDlg::StoreGetPriceCallback(int error, const wchar_t* message, void* userData)
{
    CMFCClientDlg* instance = (CMFCClientDlg*)userData;
    std::wstringstream ws;
    ws << L"Buy App: Price " << message;
    instance->SetDlgItemText(IDC_TEXT_DISPLAY, ws.str().c_str());
}

#define DESKTOP_APP_PATH "D:\\github\\MFCStoreClient\\x64\\Debug\\ConsoleApplication1.exe"

std::wstring GetAppDirectory()
{
    wchar_t path[MAX_PATH];
    GetModuleFileName(nullptr, path, sizeof(path));
    std::wstring appPath(path);
    size_t found = appPath.find_last_of(L"/\\");
    return(appPath.substr(0, found));
}

void CMFCClientDlg::OnLaunchWin32App()
{
    // Prepare handles.
    STARTUPINFO si;
    PROCESS_INFORMATION pi; // The function returns this
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    wchar_t path[MAX_PATH];
    GetModuleFileName(nullptr, path, sizeof(path));

    std::wstring args; // add your app args if needed

    //Prepare CreateProcess args
    std::wstring app_w = GetAppDirectory() + L"\\ConsoleApplication1.exe";
    std::wstring input = app_w + L" " + args;
    wchar_t* arg_concat = const_cast<wchar_t*>(input.c_str());
    const wchar_t* app_const = app_w.c_str();

    // Start the child process.
    if (!CreateProcessW(
        app_const,      // app path
        arg_concat,     // Command line (needs to include app path as first argument. args seperated by whitepace)
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        throw std::exception("Could not create child process");
    }
}


void CMFCClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMFCClientDlg::OnPaint()
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
HCURSOR CMFCClientDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

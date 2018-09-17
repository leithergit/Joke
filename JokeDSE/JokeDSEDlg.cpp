
// MyInjectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JokeDSE.h"
#include "JokeDSEDlg.h"
#include "afxdialogex.h"
#include "Utility.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
#define  __RemoteThreadFuncBodyLength	0x21B
#define  __RemoteThreadMonitorLength	0x24A
#else

#define  __RemoteThreadFuncBodyLength	0x200
#define  __RemoteThreadMonitorLength	0x200
#endif

#define __RemoteThread_Funtcion_SetTagID		0
#define __RemoteThread_Funtcion_PressButton		1

#define __INJECT_WINDOW_TITLE	_T("new 1 - Notepad++ [Administrator]")



#include<Tlhelp32.h>
using namespace std;
DWORD GetProcessIDByName(TCHAR *FileName)
{
	_tcsupr_s(FileName,MAX_PATH);
	HANDLE myhProcess;
	PROCESSENTRY32 mype;
	mype.dwSize = sizeof(PROCESSENTRY32);
	DWORD mybRet;
	//进行进程快照
	myhProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //TH32CS_SNAPPROCESS快照所有进程
	//开始进程查找
	mybRet = Process32First(myhProcess, &mype);
	//循环比较，得出ProcessID
	while (mybRet)
	{
		_tcsupr_s(mype.szExeFile,MAX_PATH);
		if (_tcscmp(FileName, mype.szExeFile) == 0)
			return mype.th32ProcessID;
		else
			mybRet = Process32Next(myhProcess, &mype);
	}
	return 0;
}

int EnableDebugPriv(const TCHAR * name)  //提升进程为DEBUG权限
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;
	//打开进程令牌环
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hToken))
	{
		MessageBox(NULL, _T("调用OpenProcessToken 失败,软件将退出"), _T("提示"), MB_OK | MB_ICONSTOP);
		return 1;
	}
	//获得进程本地唯一ID
	if (!LookupPrivilegeValue(NULL, name, &luid))
	{
		MessageBox(NULL, _T("调用LookupPrivilegeValue 失败,软件将退出"), _T("提示"), MB_OK | MB_ICONSTOP);
		return 1;

	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;
	//调整进程权限
	if (!AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		MessageBox(NULL, _T("调用AdjustTokenPrivileges 失败,软件将退出"), _T("提示"), MB_OK | MB_ICONSTOP);
		return 1;
	}
	return 0;
}

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


// CMyInjectionDlg dialog



CMyInjectionDlg::CMyInjectionDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMyInjectionDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMyInjectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMyInjectionDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_INJECT, &CMyInjectionDlg::OnBnClickedButtonInject)
	ON_BN_CLICKED(IDC_BUTTON_T, &CMyInjectionDlg::OnBnClickedButtonT)
	ON_BN_CLICKED(IDC_BUTTON_SAVECACHE, &CMyInjectionDlg::OnBnClickedButtonSavecache)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CMyInjectionDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE2, &CMyInjectionDlg::OnBnClickedButtonBrowse2)
END_MESSAGE_MAP()


// CMyInjectionDlg message handlers

BOOL CMyInjectionDlg::OnInitDialog()
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

	TCHAR szPath[1024];
	GetAppPath(szPath, 1024);
	_tcscat_s(szPath, 1024, _T("\\RecvFile"));
	m_strSavePath = szPath;
	SetDlgItemText(IDC_EDIT_SAVEPATH, m_strSavePath);
	
	m_ctlFileList.SubclassDlgItem(IDC_LIST_FILE, this);
	m_ctlFileList.SetExtendedStyle(m_ctlFileList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER /*|LVS_EX_CHECKBOXES|LVS_EX_SUBITEMIMAGES*/);
	m_ctlFileList.InsertColumn(0, _T("No"), LVCFMT_LEFT, 60);
	m_ctlFileList.InsertColumn(1, _T("Source File"), LVCFMT_LEFT, 280);
	m_ctlFileList.InsertColumn(2, _T("Dest File"), LVCFMT_LEFT, 360);
	m_ctlFileList.InsertColumn(4, _T("Size"), LVCFMT_LEFT, 100);
	m_ctlFileList.InsertColumn(5, _T("Status"), LVCFMT_LEFT, 80);
	

	m_strFilter = _T(".git .svn .obj .aps .opt .sbr .res .pdb .bsc .pch .ipch .idb .ncb .plg .ilk .exe .nlb .sdf .exp .log .tlb .dep .suo .user .lastbuildstate .opensdf BuildLog.htm unsuccessfulbuild .tlog");
	SetDlgItemText(IDC_EDIT_FILTER, m_strFilter);
	EnableJoke();
	m_hEventExcute = CreateEvent(nullptr, FALSE, FALSE, _ExecuteEvent);

	m_hEventJokeRun = CreateEvent(nullptr, FALSE, FALSE, _EnableEvent);
	if (!Start(55555))
	{
		DWORD dwError = WSAGetLastError();
		TCHAR szErrormsg[1024] = { 0 };
		LPVOID lpMsgBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);
		_stprintf(szErrormsg, _T("Failed in Starting Listen Port %d:%s"), 55555, (LPCTSTR)lpMsgBuf);
		AfxMessageBox((LPCTSTR)szErrormsg, MB_OK | MB_ICONSTOP);
		LocalFree(lpMsgBuf);
		return TRUE;
	}
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMyInjectionDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMyInjectionDlg::OnPaint()
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
HCURSOR CMyInjectionDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CMyInjectionDlg::InitInjection()
{
	if (pLoadLibraryW)
		return TRUE;
	if (EnableDebugPriv(SE_DEBUG_NAME))
	{
		ExitProcess(1);
	}
	TCHAR szText[256] = { 0 };

	HMODULE hKernel32 = ::LoadLibraryA("kernel32.dll");
	pLoadLibraryW = (LoadLibraryWProc)::GetProcAddress(hKernel32, "LoadLibraryW");
		
	FreeLibrary(hKernel32);
	TCHAR szLibPath[1024] = { 0 };
	GetAppPath(szLibPath, 1024);
	//_tcscat_s(szLibPath,1024, _T("\\Win32Lib.dll"));
	_tcscat_s(szLibPath, 1024, _T("\\JokeLib.dll"));

	TCHAR szProcessName[MAX_PATH] = { 0 };
	_tcscpy_s(szProcessName, MAX_PATH, _T("Notepad++.exe"));
	DWORD dwPID = GetProcessIDByName(szProcessName);
	m_hInjectProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	m_pRemoteBuffer = (void *)VirtualAllocEx(m_hInjectProcess, NULL, sizeof(WCHAR)*MAX_PATH, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!m_pRemoteBuffer)
	{
		AfxMessageBox( _T("挂接目标进程时调用VirtualAllocEx失败,请检查您的安全设置"));
		return FALSE;
	}
	DWORD dwWrittlen = 0;

	if (!WriteProcessMemory(m_hInjectProcess, m_pRemoteBuffer, szLibPath, sizeof(WCHAR)*MAX_PATH, &dwWrittlen))
	{
		AfxMessageBox( _T("挂接目标进程时调用WriteProcessMemory失败,请检查您的安全设置"));
		return FALSE;
	}
	DWORD dwThreadID = 0;
	HANDLE hRemoteThread = CreateRemoteThread(m_hInjectProcess,
		NULL,
		0,
		(PTHREAD_START_ROUTINE)(UINT *)pLoadLibraryW,
		m_pRemoteBuffer,
		0,
		&dwThreadID);
	if (!hRemoteThread)
	{
		AfxMessageBox( _T("挂接目标进程时调用CreateRemoteThread失败,请检查您的安全设置"));
		return FALSE;
	}

	return TRUE;
}


void CMyInjectionDlg::OnBnClickedButtonInject()
{
// 	EnableDlgItem(IDC_EDIT_SOURCEPATH, false);
// 	EnableDlgItem(IDC_EDIT_SAVEPATH, false);
// 	EnableDlgItem(IDC_EDIT_FILTER, false);
// 	EnableDlgItem(IDC_BUTTON_BROWSE, false);
// 	EnableDlgItem(IDC_BUTTON_BROWSE2, false);

	GetDlgItemText(IDC_EDIT_SAVEPATH, m_strSavePath);
	GetDlgItemText(IDC_EDIT_SOURCEPATH, m_strSourcePath);
	GetDlgItemText(IDC_EDIT_FILTER, m_strFilter);
	SetEvent(m_hEventExcute);
	ExcuteJoke(_AnsiString((LPCTSTR)m_strSourcePath, CP_ACP), _AnsiString((LPCTSTR)m_strFilter, CP_ACP));
	InitInjection();
}


void CMyInjectionDlg::OnBnClickedButtonT()
{
	ExcuteJoke(_AnsiString((LPCTSTR)m_strSourcePath, CP_ACP), _AnsiString((LPCTSTR)m_strFilter, CP_ACP));
	m_hJokeModule = LoadLibraryA("JokeLib.dll");
}


void CMyInjectionDlg::OnBnClickedButtonSavecache()
{
	ClearClient();
}


void CMyInjectionDlg::OnDestroy()
{
	__super::OnDestroy();

	if (m_hJokeModule)
		FreeLibrary(m_hJokeModule);
}


void CMyInjectionDlg::OnBnClickedButtonBrowse()
{
	CString strFilePath = _T("");
	BROWSEINFO bi;
	TCHAR Buffer[512];
	//初始化入口参数bi开始
	bi.hwndOwner = NULL;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer;//此参数如为NULL则不能显示对话框
	bi.lpszTitle = _T("选择路径");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.iImage = 0;
	//初始化入口参数bi结束
	LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框
	if (pIDList)//选择到路径(即：点了确定按钮)
	{
		SHGetPathFromIDList(pIDList, Buffer);
		//取得文件夹路径到Buffer里
		strFilePath = Buffer;//将路径保存在一个CString对象里
		SetDlgItemText(IDC_EDIT_SOURCEPATH, strFilePath);
	}
}


void CMyInjectionDlg::OnBnClickedButtonBrowse2()
{
	CString strFilePath = _T("");
	BROWSEINFO bi;
	TCHAR Buffer[512];
	//初始化入口参数bi开始
	bi.hwndOwner = NULL;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer;//此参数如为NULL则不能显示对话框
	bi.lpszTitle = _T("选择路径");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.iImage = 0;
	//初始化入口参数bi结束
	LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框
	if (pIDList)//选择到路径(即：点了确定按钮)
	{
		SHGetPathFromIDList(pIDList, Buffer);
		//取得文件夹路径到Buffer里
		strFilePath = Buffer;//将路径保存在一个CString对象里
		SetDlgItemText(IDC_EDIT_SAVEPATH, strFilePath);
	}
}

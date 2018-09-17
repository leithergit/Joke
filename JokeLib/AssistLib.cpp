// AssistLib.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "AssistLib.h"
#include "SocketServer.h"

#pragma comment(lib,"winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

#define  _ExecuteEvent	_T("Global\\{97D34EFC-A044-493C-9156-420849F34179}")
#define  _EnableEvent	_T("Global\\{EBDEB499-B8F2-4DE3-9C01-95A5D3A11778}")

#pragma data_seg("MySection")
// HANDLE hEventExcute = nullptr;
// HANDLE hEventJokeRun = nullptr;
bool bJokeMain = false;
char szSourcePath[1024] = { 0 };
char szFilter[4096] = { 0 };
#pragma data_seg()
#pragma comment(linker, "/SECTION:MySection,RWS")

// CAssistLibApp

BEGIN_MESSAGE_MAP(CAssistLibApp, CWinApp)
END_MESSAGE_MAP()


CAssistLibApp::CAssistLibApp()
{
		
}


CAssistLibApp theApp;

// CAssistLibApp initialization
void CALLBACK TimerCallBack(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CAssistLibApp *pThis = (CAssistLibApp*)dwUser;
	timeKillEvent(pThis->m_hTimer);
	pThis->m_hJokeRun = (HANDLE)_beginthreadex(nullptr, 0, pThis->ThreadJoke, pThis, 0, 0);
}


AFX_MODULE_STATE *g_pModule_State = nullptr;
BOOL CAssistLibApp::InitInstance()
{
	CWinApp::InitInstance();
	CSocketServer::InitializeWinsock();
	if (bJokeMain)
	{
		hEventJokeRun = OpenEvent(EVENT_ALL_ACCESS, FALSE, _EnableEvent);
		hEventExcute = OpenEvent(EVENT_ALL_ACCESS, FALSE, _ExecuteEvent);
		if (hEventExcute)
			timeSetEvent(100, 1, (LPTIMECALLBACK)TimerCallBack, (DWORD_PTR)this, TIME_ONESHOT | TIME_CALLBACK_FUNCTION);
	}
	
		
	return TRUE;
}

bool CAssistLibApp::CheckFilter(CString strFile, bool bExt)
{
	CString strFileupper = strFile;
	strFileupper.MakeUpper();
	CString strCompare = "";
	bool bMatched = false;
	if (!bExt)
	{
		int nPos = strFileupper.ReverseFind('\\');
		// 找到'\'并且不是最后一个字符
		if (nPos >= 0 && nPos != (strFileupper.GetLength() - 2))
			strCompare = strFileupper.Right(nPos);
	}
	else
		strCompare = PathFindExtension(strFileupper);

	if (strCompare.GetLength() > 0)
	{
		int nPos = m_strFilter.Find(strCompare);
		if (nPos >= 0)
		{
			nPos += strCompare.GetLength();
			int nLength = m_strFilter.GetLength();
			if ((m_strFilter[nPos] == ' ' || m_strFilter[nPos] == ';')
				|| m_strFilter.GetLength() == nPos)
				bMatched = true;
		}
	}
	return bMatched;
}

void CAssistLibApp::AccessFile(CString strFile, void *pParam)
{
	CAssistLibApp *pThis = (CAssistLibApp *)pParam;
	
	if (!pThis->CheckFilter(strFile) &&
		!pThis->CheckFilter(strFile, false))
	{
		CString strReletivePath = strFile.Right(strFile.GetLength() - pThis->m_nDirLength);
		SendFile(strReletivePath,strFile);
	}
}

UINT CAssistLibApp::JokeRun()
{
	shared_ptr<CSockClient> pClient = nullptr;
	while (true)
	{
		if (WaitForSingleObject(hEventJokeRun, 0) == WAIT_OBJECT_0)
			break;

		if (WaitForSingleObject(hEventExcute, 100) == WAIT_TIMEOUT)
		{
			continue;
		}
		//ResetEvent(hEventExcute);
		m_strDirectory = szSourcePath;
		m_strFilter = szFilter;
		m_nDirLength = m_strDirectory.GetLength();
		if (m_nDirLength > 0)
			AccessDirectory(m_strDirectory, AccessFile, this);

		Sleep(20);
	}
	return 0;
}

void CAssistLibApp::AccessDirectory(CString strDirectory, AccessCallBack pACB, void *p)
{
	CFileFind finder;

	if (strDirectory.GetAt(m_nDirLength - 1) == _T('\\'))
		strDirectory.SetAt(m_nDirLength - 1, _T('\\'));
	CString strFilePath = strDirectory + _T("\\*.*");
	BOOL bWorking = finder.FindFile(strFilePath);
	m_strFilter.MakeUpper();

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		CString strFile = finder.GetFilePath();
		if (finder.IsDirectory())
		{
			strFile.MakeUpper();
			if (CheckFilter(strFile) || 
				CheckFilter(strFile,false))
				continue;

			AccessDirectory(finder.GetFilePath(), pACB, p);
		}
		else
		{
			//TraceMsgA("%s File:%s\n", __FUNCTION__, finder.GetFilePath());
			pACB(finder.GetFilePath(), p);
		}
	}
}

bool CAssistLibApp::SendFile(CString strFilePath/*相对路径*/, CString strFile, WORD nPort)
{
	if (PathFileExists(strFile))
	{
		shared_ptr<CSockClient> pClient = make_shared<CSockClient>();
		if (pClient->ConnectServer(_T("127.0.0.1"), nPort, 1000) != INVALID_SOCKET)
		{
			try
			{
				CFile fileRead(strFile, CFile::modeRead);
				CHAR  szHeader[2048] = { 0 };
				CHAR szPath[1024] = { 0 };
				strcpy_s(szPath, 1024, (LPCTSTR)strFilePath);

				sprintf_s(szHeader, 2048, "HeaderLength:%08x;FileLength:%08x;FileDSE:%s;####\n", 1024, (UINT)fileRead.GetLength(), szPath);
				int nHeaderLen = strlen(szHeader);
				sprintf_s(szHeader, 2048, "HeaderLength:%08x;FileLength:%08x;FileDSE:%s;####\n", nHeaderLen, (UINT)fileRead.GetLength(), szPath);
				int nHeaderLength = 0, nFileLength = 0;

				CHAR  *szBuffer = new CHAR[(int)fileRead.GetLength() + nHeaderLen];
				shared_ptr<char> pBuffPtr(szBuffer);
				memcpy(szBuffer, szHeader, nHeaderLen);
				int nSize = fileRead.Read(&szBuffer[nHeaderLen], fileRead.GetLength());
				if (pClient->Send((byte *)szBuffer, fileRead.GetLength() + nHeaderLen) == (fileRead.GetLength() + nHeaderLen))
				{
					TraceMsgA("%s File Length = %d\tFile:%s .\n", __FUNCTION__, (int)(fileRead.GetLength() + nHeaderLen), strFile);
					return true;
				}
				else
					return false;
			}
			catch (CMemoryException* e)
			{
				TCHAR szError[1024] = { 0 };
				e->GetErrorMessage(szError, 1024);
				TraceMsgA("%s %s.\n", __FUNCTION__, szError);
				return false;
			}
			catch (CFileException* e)
			{
				TCHAR szError[1024] = { 0 };
				e->GetErrorMessage(szError, 1024);
				TraceMsgA("%s %s.\n", __FUNCTION__, szError);
				return false;
			}
			catch (CException* e)
			{
				TCHAR szError[1024] = { 0 };
				e->GetErrorMessage(szError, 1024);
				TraceMsgA("%s %s.\n", __FUNCTION__, szError);
				return false;
			}
			
		}
		else
			return false;
	}
	else
		return false;
}

void EnableJoke()
{
	bJokeMain = true;
}
void ExcuteJoke(char *szSource,char *szFilter1)
{
	strcpy_s(szSourcePath, 1024, szSource);
	strcpy_s(szFilter, 4096, szFilter1);
//	SetEvent(hEventExcute);
}

// void StopJoke()
// {
// 	SetEvent(hEventJokeRun);
// }

int CAssistLibApp::ExitInstance()
{
	SetEvent(hEventJokeRun);
	WaitForSingleObject(m_hJokeRun, 15000);
	CloseHandle(m_hJokeRun);
	return CWinApp::ExitInstance();
}

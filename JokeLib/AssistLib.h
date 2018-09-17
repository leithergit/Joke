// AssistLib.h : main header file for the AssistLib DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include <mmsystem.h>

// CAssistLibApp
// See AssistLib.cpp for the implementation of this class
//

#include "stdafx.h"

class CAssistLibApp : public CWinApp
{
public:
	CAssistLibApp();

// Overrides
public:
	HANDLE hEventExcute = nullptr;
	HANDLE hEventJokeRun = nullptr;
	MMRESULT	m_hTimer;
	virtual BOOL InitInstance();
	CCriticalSection m_csList;
	list<FileInfoPtr> m_FileList;
	CWinThread *m_pThread = nullptr;
	void PushFile(CString strPath, CString strFile)
	{
		m_csList.Lock();
		m_FileList.push_back(make_shared<FileInfo>(strPath, strFile));
		m_csList.Unlock();
	} 
	static UINT __stdcall ThreadJoke(void *p)
	{
		CAssistLibApp *pThis = (CAssistLibApp *)p;
		return pThis->JokeRun();
	}
	static void AccessFile(CString strFile, void *pParam);
	// 匹配成功时返回true
	bool CheckFilter(CString strFile,bool bExt = true);
	HANDLE m_hJokeRun = nullptr;
	CString m_strFilter;
	CString m_strDirectory;
	int m_nDirLength;
	UINT JokeRun();
	void AccessDirectory(CString szDirectory, AccessCallBack pACB, void *p);
	static bool SendFile(CString strFilePath, CString strFile, WORD nPort = 55555);
	
	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};



extern CAssistLibApp theApp;
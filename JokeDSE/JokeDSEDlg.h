
// MyInjectionDlg.h : header file
//

#pragma once
#include "resource.h"
#include "SocketServer.h"
#include "Utility.h"
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "../JokeLib/JokeLib.h"
#pragma comment(lib,"../Debug/JokeLib")
typedef HMODULE (WINAPI *LoadLibraryWProc)(_In_ LPCWSTR lpLibFileName);
typedef WINBASEAPI FARPROC (WINAPI *GetProcAddressProc)(_In_ HMODULE hModule,_In_ LPCSTR lpProcName);


#define  _ExecuteEvent	_T("Global\\{97D34EFC-A044-493C-9156-420849F34179}")
#define  _EnableEvent	_T("Global\\{EBDEB499-B8F2-4DE3-9C01-95A5D3A11778}")

class CFileClient :public CSockClient
{
public:
	int nHeaderLength = 0;
	CHAR szFilePath[1024];
	CString strFileName;
	int nFileLength = 0;
	int nSavedLength = 0;
	CFile FileSave;
	int		nRecvCount = 0;
	CFileClient(SOCKET s, sockaddr_in AddrIn)
		:CSockClient(s, AddrIn)
	{

	}
	~CFileClient()
	{
		if (FileSave.m_hFile != INVALID_HANDLE_VALUE)
			FileSave.Close();
	}
};
// CMyInjectionDlg dialog
class CMyInjectionDlg : public CDialogEx,CSocketServer
{
// Construction
public:
	CMyInjectionDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_JOKEDSE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	virtual int OnAccept(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		sockaddr_in &ClientAddr = pClient->ClientAddr;
		TraceMsgA("%s Client %d.%d.%d.%d:%d Connected.\n", __FUNCTION__,
			ClientAddr.sin_addr.S_un.S_un_b.s_b1,
			ClientAddr.sin_addr.S_un.S_un_b.s_b2,
			ClientAddr.sin_addr.S_un.S_un_b.s_b3,
			ClientAddr.sin_addr.S_un.S_un_b.s_b4,
			ClientAddr.sin_port);
// 		int nRecv = pClient->Recv();
// 		if (pClient->nBufferLength > 0)
// 		{
// 			ParserData(pClient);
// 		}
		return 0;
	}
	
	virtual CSockClient *CreateClient(SOCKET s, sockaddr_in AddrIn)
	{
		return (CSockClient *)new CFileClient(s, AddrIn);
	}

	virtual void DeleteClient(CSockClient *pClient)
	{
		if (pClient)
			delete (CFileClient *)pClient;
	}
	void ParserData(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		char *pBuffer = pClient->pRecvBuffer;
		int i = 0;
		int nParserOffset = -1;

		if (pClient->nFileLength && strlen(pClient->szFilePath) > 0)
		{
			pClient->FileSave.Write(pClient->pRecvBuffer, pClient->nBufferLength);
			pClient->nSavedLength += pClient->nBufferLength;
			if (pClient->nSavedLength == pClient->nFileLength)
			{
				pClient->FileSave.Close();
				CString strItem;
				int nCount = m_ctlFileList.GetItemCount();
				strItem.Format(_T("%d"), nCount + 1);
				m_ctlFileList.InsertItem(nCount, strItem);
				m_ctlFileList.SetItemText(nCount, 1, _UnicodeString(pClient->szFilePath, CP_ACP));
				m_ctlFileList.SetItemText(nCount, 2, pClient->strFileName);
				strItem.Format(_T("%d"), pClient->nFileLength);
				m_ctlFileList.SetItemText(nCount, 3, strItem);
				m_ctlFileList.SetItemText(nCount, 4, _T("OK"));

				//pClient->DisConnect(pClient->s);
			}
			pClient->nBufferLength = 0;
		}
		else
		{
			char *pDest = strstr(pClient->pRecvBuffer, "####\n");
			if (pDest)
			{
				int nHeadStringLength = pDest - pClient->pRecvBuffer + strlen("####\n");
				int nCount = sscanf_s(pClient->pRecvBuffer, "HeaderLength:%08x;FileLength:%08x;FileDSE:%[^;]", &pClient->nHeaderLength, &pClient->nFileLength, pClient->szFilePath, _countof(pClient->szFilePath));
				if (nCount == 3)
				{
					TraceMsgA("%s Recv = %d\tFile:%s.\n", __FUNCTION__, pClient->nBufferLength, pClient->szFilePath);
					nParserOffset = nHeadStringLength ;
					pClient->strFileName = m_strSavePath + _UnicodeString(pClient->szFilePath,CP_ACP);
					int nPos = pClient->strFileName.ReverseFind(_T('\\'));
					CString strDir = pClient->strFileName.Left(nPos);
					if (!PathFileExists((LPTSTR)(LPCTSTR)strDir))
						CreateDirectoryTree((LPTSTR)(LPCTSTR)strDir);
					pClient->FileSave.Open(pClient->strFileName, CFile::modeCreate | CFile::modeWrite);
					pClient->FileSave.Write(&pClient->pRecvBuffer[nParserOffset], pClient->nBufferLength - nParserOffset );
					pClient->nSavedLength += (pClient->nBufferLength - nParserOffset);
					if (pClient->nSavedLength == pClient->nFileLength)
					{
						pClient->FileSave.Close();
						CString strItem;
						int nCount = m_ctlFileList.GetItemCount();
						strItem.Format(_T("%d"),nCount + 1);
						m_ctlFileList.InsertItem(nCount, strItem);
						m_ctlFileList.SetItemText(nCount, 1, _UnicodeString(pClient->szFilePath,CP_ACP));
						m_ctlFileList.SetItemText(nCount, 2, pClient->strFileName);
						strItem.Format(_T("%d"), pClient->nFileLength);
						m_ctlFileList.SetItemText(nCount, 3, strItem);
						m_ctlFileList.SetItemText(nCount, 4, _T("OK"));

						//pClient->DisConnect(pClient->s);
					}
					pClient->nBufferLength = 0;
					return;
				}
			}
		}

	}

	virtual int OnRecv(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		pClient->nRecvCount++;
		int nRecv = pClient->Recv();
		TraceMsgA("%s Recv pClient->nBufferLength = %d.\n", __FUNCTION__, pClient->nBufferLength);
		if (pClient->nBufferLength > 0)
		{
			ParserData(pClient);
		}
		return nRecv;
	}

	virtual int OnWrite(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		sockaddr_in &ClientAddr = pClient->ClientAddr;
		return 0;
	}

	virtual int OnDisConnect(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		sockaddr_in &ClientAddr = pClient->ClientAddr;
		TraceMsgA("%s Client %d.%d.%d.%d:%d Disconnected,RecvCount = %d.\n", __FUNCTION__,
			ClientAddr.sin_addr.S_un.S_un_b.s_b1,
			ClientAddr.sin_addr.S_un.S_un_b.s_b2,
			ClientAddr.sin_addr.S_un.S_un_b.s_b3,
			ClientAddr.sin_addr.S_un.S_un_b.s_b4,
			ClientAddr.sin_port,
			pClient->nRecvCount);
		if (pClient->nBufferLength > 0)
		{
			ParserData(pClient);
		}
		return 0;
	}

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
	afx_msg void OnBnClickedButtonInject();
	
	BOOL InitInjection();
	CListCtrl m_ctlFileList;
	HANDLE m_hEventExcute = nullptr;
	HANDLE m_hEventJokeRun = nullptr;
	HANDLE	m_hInjectProcess;
	CString m_strSavePath;
	CString m_strSourcePath;
	CString m_strFilter;
	afx_msg void OnBnClickedButtonT();
	void			*m_pRemoteBuffer;
	LoadLibraryWProc pLoadLibraryW;
	HMODULE m_hJokeModule = nullptr;
	afx_msg void OnBnClickedButtonSavecache();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonBrowse2();

};

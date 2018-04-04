#pragma once
#include "Resource.h"
#include "WorkerThread.h"

class TaskThread :
  public WorkerThread
{
public:
  TaskThread(WORD cmd, LPSTR lpstrFile);
  ~TaskThread ();
  virtual void Terminate();
  void Work(LPVOID pWorkParam);
  void GetProgress(double& total, double& cur);
  bool IsOK() { return m_isOK; }

private:
  DWORD m_progress;
  DWORD m_count;
  WORD m_cmd;
  CString m_strFile;
  bool m_isOK, m_isAuto;
  FILE *m_fp;

  void FileSave(WORD cmd);
  void FileImport();
};

class DlgProgress :
  public CDialogImpl<DlgProgress>
{
public:
  DlgProgress(WORD cmd, LPSTR lpstrFile);
  ~DlgProgress();
  enum { IDD = IDD_PROGRESS };

  BEGIN_MSG_MAP(CAboutDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
  END_MSG_MAP()
  //break; default:  break; } stdlog("message = %x %d %d\n", uMsg, wParam, lParam); return FALSE;  }

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  void End(int wID);
  CStatic m_ctrlInfo;
  CProgressBarCtrl m_ctrlProgress;
  TaskThread* m_pTaskThread;
  WORD m_cmd;

};

#pragma once
#include "Resource.h"
#include "WorkerThread.h"

class TaskThread :
  public WorkerThread
{
    friend class DlgProgress;
public:
  TaskThread(WORD cmd, LPSTR lpstrFile, bool isAotu);
  ~TaskThread ();
  virtual void Terminate();
  void Work(LPVOID pWorkParam);
  void GetProgress(double& total, double& cur);
  bool IsOK() { return m_isOK; }

private:
  DWORD m_progress;
  DWORD m_count;
  WORD m_cmd;
  CString m_strFilePath;
  CString m_strFileName;
  CString m_strFileNameWithoutExt;
  CString m_strFileExt;
  bool m_isOK, m_isAuto;
  FILE *m_fp;

  void FileSave(WORD cmd);
  void FileImportTrases();
  void FileImportLogcat();
  bool isShortLog();
  void ImportShortLog();
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
  LPCTSTR getFileName() { return m_pTaskThread->m_strFileName.GetString(); };


  void End(int wID);
  CStatic m_ctrlInfo;
  CProgressBarCtrl m_ctrlProgress;
  TaskThread* m_pTaskThread;
  WORD m_cmd;
  bool m_isAuto;
};

#define MAX_FN_NAME_SIZE 18
struct SHORT_LOG_REC
{
    unsigned int nn;
    char log_type;
    unsigned int tid;
    unsigned int this_fn;
    unsigned int call_site;
    char cb_fn_name;
    char fn_name[MAX_FN_NAME_SIZE]; //make sure SHORT_LOG_REC is on boundry of 4
};

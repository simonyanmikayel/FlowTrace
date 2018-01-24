#include "stdafx.h"
#include "ProgressDlg.h"
#include "Helpers.h"
#include "Archive.h"
#include "Settings.h"

#pragma warning( push )
#pragma warning(disable:4477) //string '%d' requires an argument of type 'int *', but variadic argument 2 has type 'char *'

CProgressDlg::CProgressDlg(WORD cmd, LPSTR lpstrFile)
{
  m_cmd = cmd;
  m_pTaskThread = new TaskThread(cmd, lpstrFile);
}


CProgressDlg::~CProgressDlg()
{
  delete m_pTaskThread;
}

LRESULT CProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  if (!m_pTaskThread->IsOK())
  {
    End(IDCANCEL);
    return FALSE;
  }

  if (m_cmd == ID_FILE_IMPORT)
    ::SendMessage(hwndMain, WM_INPORT_TASK, 0, 0);
  m_pTaskThread->StartWork(NULL);
  SetTimer(1, 500);

  m_ctrlInfo.Attach(GetDlgItem(IDC_STATIC_INFO));
  m_ctrlProgress.Attach(GetDlgItem(IDC_PROGRESS));
  m_ctrlProgress.SetRange(0, 100);

  CenterWindow(GetParent());
  return TRUE;
}

LRESULT CProgressDlg::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  if (wParam == 1)
  { 
    if (m_pTaskThread->IsWorking())
    {
      double total, cur;
      m_pTaskThread->GetProgress(total, cur);
      CString strMsg;
      strMsg.Format(_T("%llu of %llu"), (long long)cur, (long long)total);
      m_ctrlInfo.SetWindowText(strMsg);
      if (total)
      {
        double progress = 100.0 * (cur / total);
        m_ctrlProgress.SetPos((int)progress);
      }
        
    }
    else
    {
      End(IDOK);
    }

  }
  return 0;
}

LRESULT CProgressDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  End(IDCANCEL);
  return 0;
}

void CProgressDlg::End(int wID)
{
  KillTimer(1);
  m_pTaskThread->StopWork();
  if (m_cmd == ID_FILE_IMPORT)
  {
    ::SendMessage(hwndMain, WM_INPORT_TASK, 1, 0);
  }
  EndDialog(wID);

}

///////////////////////////////////////////////////
TaskThread::TaskThread(WORD cmd, LPSTR lpstrFile)
{
  m_cmd = cmd;
  m_isOK = false;
  m_fp = NULL;
  m_progress = 0;
  m_count = 1;

  if (lpstrFile == 0)
  {
    int nRet;
    if (m_cmd == ID_FILE_IMPORT)
    {
      CFileDialog dlg(TRUE);
      nRet = dlg.DoModal();
      m_strFile = dlg.m_ofn.lpstrFile;
    }
    else
    {
      //LPCTSTR lpcstrTextFilter =
      //  _T("Text Files (*.txt)\0*.txt\0")
      //  _T("All Files (*.*)\0*.*\0")
      //  _T("");
      CFileDialog dlg(FALSE);// , NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrTextFilter);
      nRet = dlg.DoModal();
      m_strFile = dlg.m_ofn.lpstrFile;
    }
  }
  else
  {
    m_strFile = lpstrFile;
  }

  m_isAuto = false;
  if (!m_strFile.IsEmpty())
  {
    if (m_strFile == "auto")
    {
      m_isAuto = true;
    }
    else
    {
      if (m_cmd == ID_FILE_IMPORT)
      {
        if (0 != fopen_s(&m_fp, m_strFile, "r")) {
          Helpers::SysErrMessageBox(TEXT("Cannot open file %s"), m_strFile);
          m_fp = NULL;
        }
      }
      else
      {
        if (0 != fopen_s(&m_fp, m_strFile, "w")) {
          Helpers::SysErrMessageBox(TEXT("Cannot create file %s"), m_strFile);
          m_fp = NULL;
        }
      }
    }
  }
  m_isOK = m_isAuto || (m_fp != NULL);

}

TaskThread::~TaskThread()
{
  if (m_fp)
    fclose(m_fp);
}

void TaskThread::Terminate()
{
}

void TaskThread::GetProgress(double& total, double& cur)
{
  total = m_count;
  cur = m_progress;
}

void TaskThread::Work(LPVOID pWorkParam)
{
  if (m_isOK)
  {
    if (m_cmd == ID_FILE_IMPORT)
      FileImport();
    else
      FileSave(m_cmd);
  }
}

void TaskThread::FileSave(WORD cmd)
{
  bool isExport = (cmd == ID_FILE_EXPORT);

  m_count = isExport ? gArchive.getCount() : listNodes->Count();

  char buf[MAX_RECORD_LEN];
  ROW_LOG_REC* rec = (ROW_LOG_REC*)buf;

  for (DWORD i = 0; (i < m_count) && IsWorking(); i++, m_progress++)
  {
    LOG_NODE* pNode = isExport ? gArchive.getNode(i) : listNodes->getNode(i);
    if (!(pNode->isFlow() || pNode->isTrace()))
      continue;
    DWORD pc_sec, pc_msec;
    if (isExport)
    {
      int cbColor = 0;
      char szColor[32];
      if (pNode->isFlow())
      {
        FLOW_DATA* p = ((FLOW_NODE*)pNode)->getData();
        pc_sec = p->pc_sec;
        pc_msec = p->pc_msec;
        rec->log_type = p->log_type;
        rec->nn = p->nn;
        rec->sec = p->term_sec;
        rec->msec = p->term_msec;
        rec->tid = pNode->proc->getData()->tid;
        rec->app_sec = pNode->proc->getData()->pAppNode->getData()->app_sec;
        rec->app_msec = pNode->proc->getData()->pAppNode->getData()->app_msec;
        rec->this_fn = p->this_fn;
        rec->call_site = p->call_site;
        rec->call_line = p->call_line;

        rec->cb_app_path = pNode->proc->getData()->pAppNode->getData()->cb_app_path;
        memcpy(rec->appPath(), pNode->proc->getData()->pAppNode->getData()->appPath(), rec->cb_app_path);

        rec->cb_fn_name = p->cb_fn_name;
        memcpy(rec->fnName(), p->fnName(), p->cb_fn_name);

        rec->cb_trace = 0;
      }
      else
      {
        TRACE_DATA* p = ((TRACE_NODE*)pNode)->getData();
        pc_sec = p->pc_sec;
        pc_msec = p->pc_msec;
        rec->log_type = p->log_type;
        rec->nn = p->nn;
        rec->sec = p->term_sec;
        rec->msec = p->term_msec;
        rec->tid = pNode->proc->getData()->tid;
        rec->app_sec = pNode->proc->getData()->pAppNode->getData()->app_sec;
        rec->app_msec = pNode->proc->getData()->pAppNode->getData()->app_msec;
        rec->this_fn = 0;
        rec->call_site = 0;
        rec->call_line = p->call_line;

        rec->cb_app_path = pNode->proc->getData()->pAppNode->getData()->cb_app_path;
        memcpy(rec->appPath(), pNode->proc->getData()->pAppNode->getData()->appPath(), rec->cb_app_path);

        rec->cb_fn_name = p->cb_fn_name;
        memcpy(rec->fnName(), p->fnName(), p->cb_fn_name);

        char* log = pNode->getListText(&rec->cb_trace, LOG_COL);
        memcpy(rec->trace(), log, rec->cb_trace + 1);
        if (p->color)
        {
          cbColor = sprintf(szColor, "\033[0;%dm", p->color);
          rec->cb_trace += cbColor;
        }
      }
      rec->len = sizeof(ROW_LOG_REC) + rec->cbData();

      fprintf(m_fp, "%d %d %d %d %d %d %d %d %d %u %u %u %u %d %d %d-",
        rec->len, rec->log_type, rec->nn, rec->cb_app_path, rec->cb_fn_name, rec->cb_trace, rec->tid, pc_sec, pc_msec, rec->app_sec, rec->app_msec, rec->sec, rec->msec, rec->this_fn, rec->call_site, rec->call_line);

      fwrite(rec->data, rec->cbData() - cbColor, 1, m_fp);
      if (cbColor)
        fwrite(szColor, cbColor, 1, m_fp);
      fwrite("\n", 1, 1, m_fp);
    }
    else
    {
      fwrite("[", 1, 1, m_fp);
      if (pNode->isFlow())
      {
        FLOW_DATA* p = ((FLOW_NODE*)pNode)->getData();
        fwrite(pNode->proc->getData()->pAppNode->getData()->appPath(), 1, pNode->proc->getData()->pAppNode->getData()->cb_app_path, m_fp);
        fwrite(" ", 1, 1, m_fp);
        fwrite(p->fnName(), 1, p->cb_fn_name, m_fp);
        fprintf(m_fp, " %d", p->call_line);
      }
      else
      {
        TRACE_DATA* p = ((TRACE_NODE*)pNode)->getData();
        fwrite(pNode->proc->getData()->pAppNode->getData()->appPath(), 1, pNode->proc->getData()->pAppNode->getData()->cb_app_path, m_fp);
        fwrite(" ", 1, 1, m_fp);
        fwrite(p->fnName(), 1, p->cb_fn_name, m_fp);
        fprintf(m_fp, " %d", p->call_line);
      }
      fwrite("] ", 1, 2, m_fp);

      if (pNode->isFlow())
        fwrite("\n", 1, 1, m_fp);
      else
        fprintf(m_fp, "%s\n",pNode->getListText(&rec->cb_trace, LOG_COL));
    }
  }
}

void TaskThread::FileImport()
{
  DWORD pc_sec, pc_msec;

  if (m_isAuto)
  {
    m_count = 3;// 50000000;
    int cRecursion = 3;//10
    int ii = 0;
    bool ss = true;
    char sz1[] = { "86 0 0 7 18 1 3171 1466073590 349287000 1466073590 349287000 1466073590 350294000 00023230 001699F8 0-CTAPapp^thread_init_slave\n" };
    char sz2[] = { "90 2 2 7 18 5 3171 1466073590 349287000 1466073590 349287000 1466073590 350752000 00023230 001699F8 0-CTAPapp^thread_init_slavetest " };
    char sz3[] = { "86 1 3 7 18 1 3171 1466073590 349287000 1466073590 349287000 1466073590 350752000 00023230 001699F8 0-CTAPapp^thread_init_slave\n" };
    char buf1[MAX_RECORD_LEN / 2] = { 0 }, buf2[MAX_RECORD_LEN / 2] = { 0 }, buf3[MAX_RECORD_LEN / 2] = { 0 };
    ROW_LOG_REC* rec1 = (ROW_LOG_REC*)buf1;
    ROW_LOG_REC* rec2 = (ROW_LOG_REC*)buf2;
    ROW_LOG_REC* rec3 = (ROW_LOG_REC*)buf3;
    ss = (17 == (ii = sscanf(sz1, TEXT("%d %d %d %d %d %d %d %d %d %u %u %u %u %p %p %d-%s"),
      &rec1->len, &rec1->log_type, &rec1->nn, &rec1->cb_app_path, &rec1->cb_fn_name, &rec1->cb_trace, &rec1->tid, &pc_sec, &pc_msec, &rec1->app_sec, &rec1->app_msec, &rec1->sec, &rec1->msec, &rec1->this_fn, &rec1->call_site, &rec1->call_line, &rec1->data)));
    ss = (17 == sscanf(sz2, TEXT("%d %d %d %d %d %d %d %d %d %u %u %u %u %p %p %d-%s"),
      &rec2->len, &rec2->log_type, &rec2->nn, &rec2->cb_app_path, &rec2->cb_fn_name, &rec2->cb_trace, &rec2->tid, &pc_sec, &pc_msec, &rec2->app_sec, &rec2->app_msec, &rec2->sec, &rec2->msec, &rec2->this_fn, &rec2->call_site, &rec2->call_line, &rec2->data));
    ss = (17 == sscanf(sz3, TEXT("%d %d %d %d %d %d %d %d %d %u %u %u %u %p %p %d-%s"),
      &rec3->len, &rec3->log_type, &rec3->nn, &rec3->cb_app_path, &rec3->cb_fn_name, &rec3->cb_trace, &rec3->tid, &pc_sec, &pc_msec, &rec3->app_sec, &rec3->app_msec, &rec3->sec, &rec3->msec, &rec3->this_fn, &rec3->call_site, &rec3->call_line, &rec3->data));
    rec1->len = rec1->size();
    rec2->len = rec2->size(); rec2->trace()[rec2->cb_trace - 1] = '\n';
    rec3->len = rec3->size();
    ss = ss && rec1->isValid();
    ss = ss && rec2->isValid();
    for (DWORD j = 0; j < m_count; j++)
    {
      for (int i = 0; i < cRecursion; i++)
      {
        ss = ss && gArchive.append(rec1, pc_sec, pc_msec);
    	ss = ss && gArchive.append(rec2, pc_sec, pc_msec);
	  }
	  for (int i = 0; i < cRecursion; i++)
		  ss = ss && gArchive.append(rec3, pc_sec, pc_msec);
    }
  }
  else
  {
    char buf[MAX_RECORD_LEN];

    DWORD count = 0;
    fseek(m_fp, 0, SEEK_END);
    m_count = ftell(m_fp);
    fseek(m_fp, 0, SEEK_SET);
    bool ss = true;
    while (ss && IsWorking())
    {
      count++;
      if (count == 2656)
        count = count;
      ROW_LOG_REC* rec = (ROW_LOG_REC*)buf;
      ss = (16 == fscanf(m_fp, TEXT("%d %d %d %d %d %d %d %d %d %u %u %u %u %p %p %d-"),
        &rec->len, &rec->log_type, &rec->nn, &rec->cb_app_path, &rec->cb_fn_name, &rec->cb_trace, &rec->tid, &pc_sec, &pc_msec, &rec->app_sec, &rec->app_msec, &rec->sec, &rec->msec, &rec->this_fn, &rec->call_site, &rec->call_line));
      ss = ss && rec->isValid();
      if (rec->cbData())
        ss = ss && (1 == fread(rec->data, rec->cbData(), 1, m_fp));
      if (ss)
      {
        if (rec->isTrace())
        {
          rec->data[rec->cbData()] = '\n';
          rec->cb_trace++;
          rec->len++;
        }
        rec->data[rec->cbData()] = 0;
      }
      ss = ss && gArchive.append(rec, pc_sec, pc_msec);
      if (feof(m_fp))
      {
        ss = true;
        break;
      }
      m_progress = ftell(m_fp);
    }
    if (!ss)
    {
      Helpers::ErrMessageBox(TEXT("Record line %d corupted"), count);
    }
  }
}
#pragma warning(pop)

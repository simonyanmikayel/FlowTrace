#include "stdafx.h"
#include "DlgProgress.h"
#include "Helpers.h"
#include "Archive.h"
#include "Settings.h"
#include "MainFrm.h"

#pragma warning( push )
#pragma warning(disable:4477) //string '%d' requires an argument of type 'int *', but variadic argument 2 has type 'char *'

DlgProgress::DlgProgress(WORD cmd, LPSTR lpstrFile)
{
    m_cmd = cmd;
    m_pTaskThread = new TaskThread(cmd, lpstrFile);
}


DlgProgress::~DlgProgress()
{
    delete m_pTaskThread;
}

LRESULT DlgProgress::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (!m_pTaskThread->IsOK())
    {
        End(IDCANCEL);
        return FALSE;
    }

    if (m_cmd == ID_FILE_IMPORT)
        ::SendMessage(hwndMain, WM_INPORT_TASK, 0, 0);
    m_pTaskThread->StartWork(NULL);
    SetTimer(1, 2000);

    m_ctrlInfo.Attach(GetDlgItem(IDC_STATIC_INFO));
    m_ctrlProgress.Attach(GetDlgItem(IDC_PROGRESS));
    m_ctrlProgress.SetRange(0, 100);

    CenterWindow(GetParent());
    return TRUE;
}

LRESULT DlgProgress::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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
                Helpers::UpdateStatusBar();
            }

        }
        else
        {
            End(IDOK);
        }

    }
    return 0;
}

LRESULT DlgProgress::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    End(IDCANCEL);
    return 0;
}

void DlgProgress::End(int wID)
{
    KillTimer(1);
    m_pTaskThread->StopWork();
    if (m_cmd == ID_FILE_IMPORT)
    {
        ::PostMessage(hwndMain, WM_INPORT_TASK, 1, 0);
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
        INT_PTR nRet;
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

    m_count = isExport ? gArchive.getCount() : gArchive.getListedNodes()->Count();

    char buf[MAX_RECORD_LEN];
    ROW_LOG_REC* rec = (ROW_LOG_REC*)buf;

    for (DWORD i = 0; (i < m_count) && IsWorking(); i++, m_progress++)
    {
        LOG_NODE* pNode = isExport ? gArchive.getNode(i) : gArchive.getListedNodes()->getNode(i);        
        if (!pNode->isInfo())
            continue;
        INFO_NODE* pInfoNode = (INFO_NODE*)pNode;

        if (i == 1711)
            i = i;
		if (isExport)
        {
            int cbColor = 0;
            char szColor[32];
            if (pInfoNode->isFlow())
            {
				//continue;
                FLOW_NODE* p = (FLOW_NODE*)pInfoNode;
                rec->log_type = p->log_type;
				rec->log_flags = p->log_flags;
				rec->nn = p->nn;
                rec->sec = p->sec;
                rec->msec = p->msec;
                rec->tid = pInfoNode->threadNode->tid;
                rec->pid = pInfoNode->threadNode->pAppNode->pid;
                rec->this_fn = p->this_fn;
                rec->call_site = p->call_site;
				rec->fn_line = p->fn_line;
				rec->call_line = p->call_line;
            }
            else
            {
                TRACE_NODE* p = (TRACE_NODE*)pInfoNode;
                rec->log_type = p->log_type;
				rec->log_flags = p->log_flags;
				rec->nn = p->nn;
                rec->sec = p->sec;
                rec->msec = p->msec;
                rec->tid = pInfoNode->threadNode->tid;
                rec->pid = pInfoNode->threadNode->pAppNode->pid;
                rec->this_fn = 0;
                rec->call_site = 0;
				rec->fn_line = 0;
				rec->call_line = p->call_line;
				rec->cb_trace = p->cb_trace;
            }
            
            rec->cb_app_name = pInfoNode->threadNode->pAppNode->cb_app_name;
            rec->cb_module_name = pInfoNode->cb_module_name;
            rec->cb_fn_name = pInfoNode->cb_fn_name;
            memcpy((void*)rec->appName(), pInfoNode->threadNode->pAppNode->appName, rec->cb_app_name);
            if (rec->cb_module_name)
                memcpy((void*)rec->moduleName(), pInfoNode->moduleName(), rec->cb_module_name);
            memcpy((void*)rec->fnName(), pInfoNode->fnName(), rec->cb_fn_name);
			//gArchive.Log(rec);
            if (pInfoNode->cb_java_call_site && (pInfoNode->log_flags & LOG_FLAG_JAVA))
            {
                rec->cb_java_call_site = pInfoNode->cb_java_call_site;
                memcpy((void*)rec->trace(), pInfoNode->JavaCallSite(), pInfoNode->cb_java_call_site);
            }
            else //if (rec->cb_trace)
            {
                TRACE_NODE* p = (TRACE_NODE*)pInfoNode;
                int cb_trace;
                char* log = pInfoNode->getListText(&cb_trace, LOG_COL);
                rec->cb_trace = cb_trace;
                int cb_size = rec->size();
                bool truncate = cb_size > MAX_RECORD_LEN - 128;
                if (truncate) {
                    cb_trace -= (cb_size - MAX_RECORD_LEN + 128);
                    rec->cb_trace = cb_trace;
                }
                memcpy((void*)rec->trace(), log, rec->cb_trace + 1);
                if (truncate) {
                    memcpy((void*)(rec->trace() + rec->cb_trace), "...", 3);
                    rec->cb_trace += 3;
                }
                if (p->color)
                {
                    cbColor = sprintf_s(szColor, _countof(szColor), "\033[0;%dm", p->color);
                    rec->cb_trace += cbColor;
                }
                if (rec->cb_trace > 2000)
                    i = i;
            }

            rec->len = sizeof(ROW_LOG_REC) + rec->cbData();

            fprintf(m_fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d-",
                rec->len, rec->log_type, rec->log_flags, rec->nn, rec->cb_app_name, rec->cb_module_name,
				rec->cb_fn_name, rec->cb_trace, rec->pid, rec->tid,
				rec->sec, rec->msec, 
				rec->this_fn, rec->call_site, rec->fn_line, rec->call_line, pInfoNode->bookmark);

            //!!fwrite(rec->data, rec->cbData() - cbColor, 1, m_fp);
            if (cbColor)
                fwrite(szColor, cbColor, 1, m_fp);
            fwrite("\n", 1, 1, m_fp);
        }
        else
        {
            fwrite("[", 1, 1, m_fp);
            if (pInfoNode->isFlow())
            {
                FLOW_NODE* p = (FLOW_NODE*)pInfoNode;
                fwrite(pInfoNode->threadNode->pAppNode->appName, 1, pInfoNode->threadNode->pAppNode->cb_app_name, m_fp);
                fwrite(" ", 1, 1, m_fp);
                fwrite(p->fnName(), 1, p->cb_fn_name, m_fp);
                fprintf(m_fp, " %d", p->call_line);
            }
            else
            {
                TRACE_NODE* p = (TRACE_NODE*)pInfoNode;
                fwrite(pInfoNode->threadNode->pAppNode->appName, 1, pInfoNode->threadNode->pAppNode->cb_app_name, m_fp);
                fwrite(" ", 1, 1, m_fp);
                fwrite(p->fnName(), 1, p->cb_fn_name, m_fp);
                fprintf(m_fp, " %d", p->call_line);
            }
            fwrite("] ", 1, 2, m_fp);

            if (pInfoNode->isFlow())
                fwrite("\n", 1, 1, m_fp);
            else {
                int cb_trace;
                fprintf(m_fp, "%s\n", pInfoNode->getListText(&cb_trace, LOG_COL));
            }
        }
    }
}

void TaskThread::FileImport()
{
    if (m_isAuto)
    {
#ifdef _BUILD_X64 
        m_count = 3;// 16LL * 1024 * 1024;// 128000000;
#else
        m_count = 3;// 16LL * 1024 * 1024;// 12800000;
#endif
        int cRecursion = 3;//10
        int ii = 0;
        bool ss = true;
        char log_buf[3][1024];
        ZeroMemory(log_buf, sizeof(log_buf));
        char* appName = "app_name";
        char* moduleName = "libAppSel.so";
        char* fnName = "fn_name";
        char* trace= "trace\n";

        ROW_LOG_REC* rec1 = (ROW_LOG_REC*)log_buf[0];
        rec1->log_type = LOG_TYPE_ENTER;
        rec1->cb_app_name = (WORD)strlen(appName);
        rec1->cb_module_name = (WORD)strlen(moduleName);
        rec1->cb_fn_name = (WORD)strlen(fnName);
        rec1->this_fn = 0x19800;
        rec1->call_site = 0x199F4;
        memcpy((void*)rec1->appName(), appName, rec1->cb_app_name);
        memcpy((void*)rec1->moduleName(), moduleName, rec1->cb_module_name);
        memcpy((void*)rec1->fnName(), fnName, rec1->cb_fn_name);
        rec1->len = rec1->size();

        ROW_LOG_REC* rec2 = (ROW_LOG_REC*)log_buf[1];
        memcpy(rec2, rec1, rec1->len);
        rec2->log_type = LOG_TYPE_TRACE;
        rec2->cb_trace = (WORD)strlen(trace);
        memcpy((void*)rec2->trace(), trace, rec2->cb_trace);
        rec2->len = rec2->size();

        ROW_LOG_REC* rec3 = (ROW_LOG_REC*)log_buf[2];
        memcpy(rec3, rec1, rec1->len);
        rec3->log_type = LOG_TYPE_EXIT;

        ss = ss && rec1->isValid();
        ss = ss && rec2->isValid();
        ss = ss && rec3->isValid();
        int nn = 0;
        for (DWORD j = 0; ss && j < m_count && IsWorking(); j++)
        {
            m_progress = j;
            for (int i = 0; i < cRecursion; i++)
            {
                rec1->nn = ++nn;
                ss = ss && gArchive.append(rec1);
                rec2->nn = ++nn;
                ss = ss && gArchive.append(rec2);
            }
            for (int i = 0; i < cRecursion; i++)
            {
                rec3->nn = ++nn;
                ss = ss && gArchive.append(rec3);
            }
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
        bool appended = true;
        while (ss && IsWorking())
        {
            count++;
            if (count == 23811)
                count = count;
            ROW_LOG_REC* rec = (ROW_LOG_REC*)buf;
            int bookmark; 
			int cf = fscanf_s(m_fp, TEXT("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d-"),
				&rec->len, &rec->log_type, &rec->log_flags, &rec->nn, 
                &rec->cb_app_name, &rec->cb_module_name, 
                &rec->cb_fn_name, &rec->cb_trace, &rec->pid, &rec->tid, &rec->sec, &rec->msec,
                &rec->this_fn, &rec->call_site, &rec->fn_line, &rec->call_line, &bookmark);
            ss = (17 == cf);
            ss = ss && rec->isValid();
			//!! if (rec->cbData())
				//!! ss = ss && (1 == fread(rec->data, rec->cbData(), 1, m_fp));
            if (ss)
            {
                if (rec->isTrace())
                {
					//!! rec->data[rec->cbData()] = '\n';
                    rec->cb_trace++;
                    rec->len++;
                }
				//!! rec->data[rec->cbData()] = 0;
            }

            ss = ss && rec->isValid();
            if (ss)
                appended = gArchive.append(rec, NULL, true, bookmark);
            ss = ss && appended;
            if (feof(m_fp))
            {
                ss = true;
                break;
            }
            m_progress = ftell(m_fp);
        }
        if (!ss)
        {
            if (!appended)
                Helpers::ErrMessageBox(TEXT("Not enought memory to uppend record %d"), count);
            else
                Helpers::ErrMessageBox(TEXT("Record line %d corupted"), count);
        }
    }
}
#pragma warning(pop)

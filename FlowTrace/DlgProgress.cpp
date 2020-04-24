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
	m_isAuto = lpstrFile && strcmp("auto", lpstrFile) == 0;
	m_pTaskThread = new TaskThread(cmd, lpstrFile, m_isAuto);
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
        ::SendMessage(hwndMain, WM_INPORT_TASK, 0, m_isAuto);
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
TaskThread::TaskThread(WORD cmd, LPSTR lpstrFile, bool isAotu)
{
    m_cmd = cmd;
    m_isOK = false;
    m_fp = NULL;
    m_progress = 0;
    m_count = 1;
	m_isAuto = isAotu;

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

    if (!m_strFile.IsEmpty() && !m_isAuto)
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

    m_count = isExport ? gArchive.getNodeCount() : gArchive.getListedNodes()->Count();

    char buf[MAX_RECORD_LEN];
    LOG_REC_NET_DATA* pLogData = (LOG_REC_NET_DATA*)buf;

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
            if (pInfoNode->isFlow())
            {
				//continue;
                FLOW_NODE* p = (FLOW_NODE*)pInfoNode;
                pLogData->log_type = p->log_type;
				pLogData->log_flags = p->log_flags;
				pLogData->nn = p->nn;
                pLogData->sec = p->sec;
                pLogData->msec = p->msec;
                pLogData->tid = pInfoNode->threadNode->tid;
                pLogData->pid = pInfoNode->threadNode->pAppNode->pid;
                pLogData->this_fn = p->this_fn;
                pLogData->call_site = p->call_site;
				pLogData->fn_line = p->fn_line;
				pLogData->call_line = p->call_line;
            }
            else
            {
                TRACE_NODE* p = (TRACE_NODE*)pInfoNode;
                pLogData->log_type = p->log_type;
				pLogData->log_flags = p->log_flags;
				pLogData->nn = p->nn;
                pLogData->sec = p->sec;
                pLogData->msec = p->msec;
                pLogData->tid = pInfoNode->threadNode->tid;
                pLogData->pid = pInfoNode->threadNode->pAppNode->pid;
                pLogData->this_fn = 0;
                pLogData->call_site = 0;
				pLogData->fn_line = 0;
				pLogData->call_line = p->call_line;
            }
            
            pLogData->cb_app_name = pInfoNode->threadNode->pAppNode->cb_app_name;
            pLogData->cb_module_name = pInfoNode->cb_module_name;
            pLogData->cb_fn_name = pInfoNode->cb_fn_name;
            memcpy((void*)pLogData->appName(), pInfoNode->threadNode->pAppNode->appName, pLogData->cb_app_name);
            if (pLogData->cb_module_name)
                memcpy((void*)pLogData->moduleName(), pInfoNode->moduleName(), pLogData->cb_module_name);
            memcpy((void*)pLogData->fnName(), pInfoNode->fnName(), pLogData->cb_fn_name);
			//gArchive.Log(pLogData);
            pLogData->cb_trace = 0;
            if (!pInfoNode->isFlow())
            {
                //TRACE_NODE* p = (TRACE_NODE*)pInfoNode;
                int cb_trace = 0;
                char* log = pInfoNode->getListText(&cb_trace, LOG_COL);
                int cb_size = pLogData->size();
                bool truncate = (cb_size + cb_trace ) > MAX_RECORD_LEN - 128;
                if (truncate) {
                    cb_trace -= (cb_size + cb_trace - MAX_RECORD_LEN + 128);
                }
                memcpy((void*)pLogData->trace(), log, cb_trace);
                if (truncate) {
                    memcpy((void*)(pLogData->trace() + cb_trace), "...", 3);
                    cb_trace += 3;
                }
                pLogData->cb_trace = cb_trace;
            }

            pLogData->len = sizeof(LOG_REC_NET_DATA) + pLogData->cbData();

            fprintf(m_fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d-",
                pLogData->len, pLogData->log_type, pLogData->log_flags, pLogData->nn,
                pLogData->cb_app_name, pLogData->cb_module_name,
                pLogData->cb_fn_name, pLogData->cb_trace, pLogData->pid, pLogData->tid, pLogData->sec, pLogData->msec,
                pLogData->this_fn, pLogData->call_site, pLogData->fn_line, pLogData->call_line, pInfoNode->bookmark);

            fwrite(pLogData->data, pLogData->cbData(), 1, m_fp);
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
                pInfoNode->isEnter() ? fwrite(" ->\n", 4, 1, m_fp) : fwrite(" <-\n", 4, 1, m_fp);
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
        m_count = 4;// 16LL * 1024 * 1024;// 128000000;
#else
        m_count = 3;// 16LL * 1024 * 1024;// 12800000;
#endif
        int cRecursion = 2;//10
        int ii = 0;
        bool ss = true;
        char log_buf[3][1024];
        ZeroMemory(log_buf, sizeof(log_buf));
        char* appName = "app_name";
        char* moduleName = "libAppSel.so";
        char* fnName = "fn_name";
        char* trace= "trace1 trace2 trace3 trace4 trace5 trace6 trace7 trace8 trace9 trace10\n";

        LOG_REC_NET_DATA* recEnter = (LOG_REC_NET_DATA*)log_buf[0];
        recEnter->log_type = LOG_TYPE_ENTER;
        recEnter->cb_app_name = (WORD)strlen(appName);
        recEnter->cb_module_name = (WORD)strlen(moduleName);
        recEnter->cb_fn_name = (WORD)strlen(fnName);
        recEnter->this_fn = 0x19800;
        recEnter->call_site = 0x199F4;
        memcpy((void*)recEnter->appName(), appName, recEnter->cb_app_name);
        memcpy((void*)recEnter->moduleName(), moduleName, recEnter->cb_module_name);
        memcpy((void*)recEnter->fnName(), fnName, recEnter->cb_fn_name);
        recEnter->len = recEnter->size();

        LOG_REC_NET_DATA* recTrace = (LOG_REC_NET_DATA*)log_buf[1];
        memcpy(recTrace, recEnter, recEnter->len);
        recTrace->log_type = LOG_TYPE_TRACE;
		recTrace->log_flags |= LOG_FLAG_NEW_LINE;
		recTrace->priority = 0;
        recTrace->cb_trace = (WORD)strlen(trace);
        memcpy((void*)recTrace->trace(), trace, recTrace->cb_trace);
        recTrace->len = recTrace->size();

        LOG_REC_NET_DATA* recExit = (LOG_REC_NET_DATA*)log_buf[2];
        memcpy(recExit, recEnter, recEnter->len);
        recExit->log_type = LOG_TYPE_EXIT;

        ss = ss && recEnter->isValid();
        ss = ss && recTrace->isValid();
        ss = ss && recExit->isValid();
        int nn = 0;
        for (DWORD j = 0; ss && j < m_count && IsWorking(); j++)
        {
            m_progress = j;
            for (int i = 0; i < cRecursion; i++)
            {
                recEnter->nn = ++nn;
                ss = ss && gArchive.append(recEnter);
                recTrace->nn = ++nn;
                ss = ss && gArchive.append(recTrace);
            }
            for (int i = 0; i < cRecursion; i++)
            {
                recExit->nn = ++nn;
                ss = ss && gArchive.append(recExit);
            }
        }
    }
    else
    {
        char buf[MAX_RECORD_LEN + 1];
        char* bufEnd = buf + MAX_RECORD_LEN;

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
            LOG_REC_NET_DATA* pLogData = (LOG_REC_NET_DATA*)buf;
            int bookmark; 
			int cf = fscanf_s(m_fp, TEXT("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d-"),
				&pLogData->len, &pLogData->log_type, &pLogData->log_flags, &pLogData->nn, 
                &pLogData->cb_app_name, &pLogData->cb_module_name, 
                &pLogData->cb_fn_name, &pLogData->cb_trace, &pLogData->pid, &pLogData->tid, &pLogData->sec, &pLogData->msec,
                &pLogData->this_fn, &pLogData->call_site, &pLogData->fn_line, &pLogData->call_line, &bookmark);

            ss = (17 == cf);
            ss = ss && pLogData->isValid();
            int cb = pLogData->cbData();
            if (bufEnd < pLogData->data + cb)
                ss = false;
            pLogData->data[cb] = 0;
			if (ss && pLogData->cbData())
			    ss = ss && (1 == fread(pLogData->data, pLogData->cbData(), 1, m_fp));
            if (ss)
            {
                if (pLogData->isTrace())
                {
					pLogData->data[pLogData->cbData()] = '\n';
                    pLogData->cb_trace++;
                    pLogData->len++;
                }
				pLogData->data[pLogData->cbData()] = 0;
            }

            ss = ss && pLogData->isValid();
            if (ss)
                appended = gArchive.append(pLogData, NULL, true, bookmark);
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

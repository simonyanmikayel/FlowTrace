#include "stdafx.h"
#include "DlgProgress.h"
#include "Helpers.h"
#include "Archive.h"
#include "Settings.h"
#include "FileMap.h"
#include "MainFrm.h"
#include "addr2line.h"

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

    if (m_cmd == ID_FILE_IMPORT_TRACES || m_cmd == ID_FILE_IMPORT_LOGCAT)
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
    if (m_cmd == ID_FILE_IMPORT_TRACES || m_cmd == ID_FILE_IMPORT_LOGCAT)
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
        if (m_cmd == ID_FILE_IMPORT_TRACES || m_cmd == ID_FILE_IMPORT_LOGCAT)
        {
            CFileDialog dlg(TRUE);
            nRet = dlg.DoModal();
            m_strFilePath = dlg.m_ofn.lpstrFile;
        }
        else
        {
            //LPCTSTR lpcstrTextFilter =
            //  _T("Text Files (*.txt)\0*.txt\0")
            //  _T("All Files (*.*)\0*.*\0")
            //  _T("");
            CFileDialog dlg(FALSE);// , NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrTextFilter);
            nRet = dlg.DoModal();
            m_strFilePath = dlg.m_ofn.lpstrFile;
        }
    }
    else
    {
        m_strFilePath = lpstrFile;
    }

    int chPos = m_strFilePath.ReverseFind('\\');
    if (chPos < 0)
        chPos = m_strFilePath.ReverseFind('/');
    if (chPos > 0)
        m_strFileName = m_strFilePath.Mid(chPos + 1);
    else
        m_strFileName = m_strFilePath;
    chPos = m_strFileName.ReverseFind('.');
    if (chPos > 0) {
        m_strFileExt = m_strFileName.Mid(chPos + 1);
        m_strFileNameWithoutExt = m_strFileName.Mid(0, chPos);
    }
    else
        m_strFileNameWithoutExt = m_strFileName;
    
    m_fp = NULL;
    if (m_isAuto)
    {
        m_isOK = true;
    }
    else if (!m_strFilePath.IsEmpty())
    {
		if (m_cmd == ID_FILE_IMPORT_TRACES || m_cmd == ID_FILE_IMPORT_LOGCAT)
		{
            if (isShortLog() || m_cmd == ID_FILE_IMPORT_LOGCAT)
            {
                m_isOK = true;
            }
            else if (0 == fopen_s(&m_fp, m_strFilePath, "rb")) {
                m_isOK = true;
			}
            else
            {
                Helpers::SysErrMessageBox(TEXT("Cannot open file %s"), m_strFilePath);
                m_fp = NULL;
            }
		}
		else
		{
			if (0 == fopen_s(&m_fp, m_strFilePath, "wb")) {
                m_isOK = true;
            }
            else
            {
                Helpers::SysErrMessageBox(TEXT("Cannot create file %s"), m_strFilePath);
                m_fp = NULL;
            }
		}
	}
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
        if (isShortLog())
            ImportShortLog();
        else if (m_cmd == ID_FILE_IMPORT_TRACES)
            FileImportTrases();
        else if (m_cmd == ID_FILE_IMPORT_LOGCAT)
            FileImportLogcat();
        else
            FileSave(m_cmd);
    }
}

void TaskThread::FileSave(WORD cmd)
{
    bool isExport = (cmd == ID_FILE_EXPORT_TRACES);

    m_count = isExport ? gArchive.getNodeCount() : gArchive.getListedNodes()->Count();

    char buf[MAX_RECORD_LEN];
    LOG_REC_NET_DATA* pLogData = (LOG_REC_NET_DATA*)buf;

    for (DWORD i = 0; (i < m_count) && IsWorking(); i++, m_progress++)
    {
        LOG_NODE* pNode = isExport ? gArchive.getNode(i) : gArchive.getListedNodes()->getNode(i);
        APP_NODE* pAppNode = 0;
        THREAD_NODE* pThreadNode = 0;
        INFO_NODE* pInfoNode = 0;
        if (pNode->isApp()) {
            pAppNode = (APP_NODE*)pNode;
        }
        else if (pNode->isThread()) {
            pThreadNode = (THREAD_NODE*)pNode;
            pAppNode = pThreadNode->pAppNode;
        }
        else if (pNode->isInfo()) {
            pInfoNode = (INFO_NODE*)pNode;
            pThreadNode = pInfoNode->threadNode;
            pAppNode = pThreadNode->pAppNode;
        }

        if (pAppNode == 0)
            continue;

		if (isExport)
        {
            if (pNode->isApp())
            {
                APP_NODE* p = (APP_NODE*)pNode;
                ZeroMemory(pLogData, sizeof(*pLogData));
                pLogData->log_type = LOG_TYPE_APP;
                pLogData->nn = p->psNN;
                pLogData->pid = pAppNode->pid;
            }
            else if (pNode->isThread())
            {
                THREAD_NODE* p = (THREAD_NODE*)pNode;
                ZeroMemory(pLogData, sizeof(*pLogData));
                pLogData->log_type = LOG_TYPE_THREAD;
                pLogData->nn = p->psNN;
                pLogData->pid = pAppNode->pid;
                pLogData->tid = pThreadNode->tid;
            }
            else if (pNode->isFlow())
            {
				//continue;
                FLOW_NODE* p = (FLOW_NODE*)pNode;
                pLogData->log_type = p->log_type;
				pLogData->log_flags = p->log_flags;
				pLogData->nn = p->nn;
                pLogData->sec = p->sec;
                pLogData->msec = p->msec;
                pLogData->tid = pThreadNode->tid;
                pLogData->pid = pAppNode->pid;
                pLogData->this_fn = p->this_fn;
                pLogData->call_site = p->call_site;
				pLogData->fn_line = p->fn_line;
				pLogData->call_line = p->call_line;
                pLogData->color = 0;
            }
            else if (pNode->isTrace())
            {
                TRACE_NODE* p = (TRACE_NODE*)pNode;
                pLogData->log_type = p->log_type;
				pLogData->log_flags = p->log_flags;
				pLogData->nn = p->nn;
                pLogData->sec = p->sec;
                pLogData->msec = p->msec;
                pLogData->tid = pThreadNode->tid;
                pLogData->pid = pAppNode->pid;
                pLogData->this_fn = 0;
                pLogData->call_site = 0;
				pLogData->fn_line = 0;
				pLogData->call_line = p->call_line;
                pLogData->color = p->color;
            }
            else
            {
                continue;
            }
            
            pLogData->cb_app_name = pAppNode->cb_app_name;
            memcpy((void*)pLogData->appName(), pAppNode->appName, pLogData->cb_app_name);
            if (pThreadNode && pLogData->log_type == LOG_TYPE_THREAD && pThreadNode->cb_thread_name) {
                pLogData->cb_fn_name = pThreadNode->cb_thread_name;
                memcpy((void*)pLogData->fnName(), pThreadNode->threadName, pLogData->cb_fn_name);
            }
            else if (pInfoNode) {
                pLogData->cb_module_name = pInfoNode->cb_module_name;
                memcpy((void*)pLogData->moduleName(), pInfoNode->moduleName(), pLogData->cb_module_name);
                pLogData->cb_fn_name = pInfoNode->cb_fn_name;
                memcpy((void*)pLogData->fnName(), pInfoNode->fnName(), pLogData->cb_fn_name);
            }

			//gArchive.Log(pLogData);
            pLogData->cb_trace = 0;
            if (pInfoNode && !pInfoNode->isFlow())
            {
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

            fprintf(m_fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d-",
                pLogData->len, pLogData->log_type, pLogData->log_flags, pLogData->color, pLogData->nn,
                pLogData->cb_app_name, pLogData->cb_module_name,
                pLogData->cb_fn_name, pLogData->cb_trace, pLogData->pid, pLogData->tid, pLogData->sec, pLogData->msec,
                pLogData->this_fn, pLogData->call_site, pLogData->fn_line, pLogData->call_line, pNode->bookmark);

            fwrite(pLogData->data, pLogData->cbData(), 1, m_fp);
            fwrite("\n", 1, 1, m_fp);
        }
        else
        {
            if (pNode->isFlow())
            {
                fwrite("[", 1, 1, m_fp);
                FLOW_NODE* p = (FLOW_NODE*)pNode;
                fwrite(pAppNode->appName, 1, pAppNode->cb_app_name, m_fp);
                fwrite(" ", 1, 1, m_fp);
                fwrite(p->fnName(), 1, p->cb_fn_name, m_fp);
                fprintf(m_fp, " %d", p->call_line);
                fwrite("] ", 1, 2, m_fp);
                p->isEnter() ? fwrite(" ->\n", 4, 1, m_fp) : fwrite(" <-\n", 4, 1, m_fp);
            }
            else if (pNode->isTrace())
            {
                fwrite("[", 1, 1, m_fp);
                TRACE_NODE* p = (TRACE_NODE*)pNode;
                fwrite(pAppNode->appName, 1, pAppNode->cb_app_name, m_fp);
                fwrite(" ", 1, 1, m_fp);
                fwrite(p->fnName(), 1, p->cb_fn_name, m_fp);
                fprintf(m_fp, " %d", p->call_line);
                fwrite("] ", 1, 2, m_fp);
                int cb_trace;
                fprintf(m_fp, "%s\n", p->getListText(&cb_trace, LOG_COL));
            }
            else
            {
                continue;
            }
        }
    }
}

void TaskThread::FileImportLogcat()
{
    static LOG_REC_ADB_DATA adbRec;

    FILE_MAP fmap(m_strFilePath, false);
    if (!fmap.Size()) {
        Helpers::ErrMessageBox(TEXT("Could not open file %s"), m_strFilePath.GetString());
        return;
    }

    m_count = fmap.Size();
    char* p = fmap.m_ptr;
    int c = (int)fmap.Size();
    int i = 0; 
    while (i < c)
    {
        Helpers::SkipWhite(p, c, i);
        adbRec.reset();
        int i0 = i;
        bool ok = true;
        int unused1 = Helpers::NextDigit(p, c, i, ok);
        ok = ok && p[i] == '-';
        int unused2 = Helpers::NextDigit(p, c, i, ok);
        ok = ok && p[i] == ' ';
        int h = Helpers::NextDigit(p, c, i, ok);
        ok = ok && p[i] == ':';
        int m = Helpers::NextDigit(p, c, i, ok);
        ok = ok && p[i] == ':';
        int s = Helpers::NextDigit(p, c, i, ok);
        ok = ok && p[i] == '.';
        int ms = Helpers::NextDigit(p, c, i, ok);
        ok = ok && p[i] == ' ';
        int pid = Helpers::NextDigit(p, c, i, ok);
        ok = ok && (p[i] == ' ' || p[i] == '-');
        int tid = Helpers::NextDigit(p, c, i, ok);
        ok = ok && (p[i] == ' ' || p[i] == '/');
        if (ok && p[i] == '/')
        {
            //we have process name TODO -use it
            Helpers::ToChar(' ', p, c, i);
        }
        Helpers::ToChar(' ', p, c, i);
        Helpers::SkipWhite(p, c, i);
        int i1 = i;
        Helpers::LineEnd(p, c, i);
        int i2 = i;
        if (ok)
        {
            adbRec.pid = pid;
            adbRec.tid = tid;
            adbRec.sec = Helpers::GetSec(h, m, s); // 3600 * h + 60 * m + s;
            adbRec.msec = ms;

            if (p[i1] == 'E')
                adbRec.priority = FLOW_LOG_ERROR;
            else if (p[i1] == 'I')
                adbRec.priority = FLOW_LOG_INFO;
            else if (p[i1] == 'D')
                adbRec.priority = FLOW_LOG_DEBUG;
            else if (p[i1] == 'W')
                adbRec.priority = FLOW_LOG_WARN;

            adbRec.p_trace = &p[i1];
            adbRec.cb_trace = (WORD)(i2-i1);
            adbRec.len = sizeof(LOG_REC_BASE_DATA) + adbRec.cbData();
            gArchive.appendAdb(&adbRec, true);
        }
        else
        {
            adbRec.p_trace = &p[i0];
            adbRec.cb_trace = (WORD)(i2 - i0);
            adbRec.len = sizeof(LOG_REC_BASE_DATA) + adbRec.cbData();
            gArchive.appendAdb(&adbRec, true);
        }

        Helpers::NextLine(p, c, i);
        m_progress = i;
    }
}

void TaskThread::FileImportTrases()
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
                ss = ss && gArchive.appendNet(recEnter);
                recTrace->nn = ++nn;
                ss = ss && gArchive.appendNet(recTrace);
            }
            for (int i = 0; i < cRecursion; i++)
            {
                recExit->nn = ++nn;
                ss = ss && gArchive.appendNet(recExit);
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
			int cf = fscanf_s(m_fp, TEXT("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d-"),
				&pLogData->len, &pLogData->log_type, &pLogData->log_flags, &pLogData->color, &pLogData->nn,
                &pLogData->cb_app_name, &pLogData->cb_module_name, 
                &pLogData->cb_fn_name, &pLogData->cb_trace, &pLogData->pid, &pLogData->tid, &pLogData->sec, &pLogData->msec,
                &pLogData->this_fn, &pLogData->call_site, &pLogData->fn_line, &pLogData->call_line, &bookmark);

            ss = (18 == cf);
            ss = ss && pLogData->isValid();
            if (ss) {
                int cb = pLogData->cbData();
                if (bufEnd < pLogData->data + cb)
                    ss = false;
                if (ss)
                    pLogData->data[cb] = 0;
            }
            ss = ss && pLogData->cbData();
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
                appended = gArchive.appendNet(pLogData, NULL, true, bookmark);
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

bool TaskThread::isShortLog()
{
    int i = _stricmp("slog", m_strFileExt.GetString());
    return 0 == i;
}

static const int max_fn_name = 32;
struct FUNC_INFO{
    unsigned int this_fn;
    char fn_name[max_fn_name + 1];
};
int compareFuncInfo(const void* a, const void* b)
{
    FUNC_INFO* p1 = (FUNC_INFO*)a;
    FUNC_INFO* p2 = (FUNC_INFO*)b;
    if (p1->this_fn == p2->this_fn)
        return 0;
    else if (p1->this_fn < p2->this_fn)
        return -1;
    else
        return 1;
}
static unsigned int fn_addr;
static FUNC_INFO* pfuncInfo;
static int fnCount;
static bool getFnCount;
static int line_info( 
    unsigned long address,
    unsigned char op_index,
    char* src,
    char* section_name,
    char* function_name,
    unsigned int line)
{
    if (fn_addr == INFINITE) {
        return 0;
    }
    if (fn_addr > address) { //esure that pfuncInfo will be sorted
        fn_addr = -1;
        return 0;
    }
    fn_addr = address;
     
    if (!function_name) {
        //stdlog("%x %s:%d %s  %s\n", address, src, line, section_name ? section_name : "?", function_name ? function_name : "??");
        return 1;
    }
    if (address == 0x11DBC4) {
        stdlog("%x %s:%d %s  %s\n", address, src, line, section_name ? section_name : "?", function_name ? function_name : "??");
    }
    if (getFnCount) {
        fnCount++;
        return 1;
    }
    pfuncInfo[fnCount].this_fn = address;
    strncpy_s(pfuncInfo[fnCount].fn_name, function_name, max_fn_name);
    pfuncInfo[fnCount].fn_name[max_fn_name] = 0;

    fnCount++;
    return 1;
}

void TaskThread::ImportShortLog()
{
    FILE_MAP fmap(m_strFilePath, false);
    SHORT_LOG_REC* pRec = (SHORT_LOG_REC*)fmap.m_ptr;
    int cRec = fmap.Size() / sizeof(SHORT_LOG_REC);
    if (!pRec || !cRec) {
        Helpers::ErrMessageBox(TEXT("Could not open file %s"), m_strFilePath.GetString());
        return;
    }

    //find first record in cyclic buffer
    int iFirst = 0;
    for (int i = 0; i < cRec - 1; i++) {
        if (pRec[i].nn == pRec[i + 1].nn) {
            Helpers::ErrMessageBox(TEXT("Bad record index %d"), i);
            goto Cleanup;
        }
        if (pRec[i].nn > pRec[i + 1].nn) {
            iFirst = i + 1;
            break;
        }
    }
    
    //fill records
    char buf[MAX_RECORD_LEN + 1];
    char* bufEnd = buf + MAX_RECORD_LEN;
    for (int c = 0, i = iFirst; c < cRec; c++) {

        int cb_app_name = m_strFileNameWithoutExt.GetLength();
        /*
        char* fn_name = "?";        
        FUNC_INFO* pfnInfo = (FUNC_INFO*)bsearch(&(pRec[i].this_fn), pfuncInfo, fnCount, sizeof(FUNC_INFO), compareFuncInfo);
        if (pfnInfo && pfnInfo->fn_name) {
            fn_name = pfnInfo->fn_name;
        }
        int cb_fn_name = (int)strlen(fn_name);
        */
        ZeroMemory(buf, sizeof(buf));
        LOG_REC_NET_DATA* pLogData = (LOG_REC_NET_DATA*)buf;
        pLogData->len = sizeof(LOG_REC_BASE_DATA) + cb_app_name + pRec[i].cb_fn_name + 1;
        pLogData->log_type = pRec[i].log_type;
        pLogData->nn = pRec[i].nn;
        pLogData->cb_app_name = cb_app_name;
        pLogData->cb_fn_name = pRec[i].cb_fn_name;
        pLogData->tid = pRec[i].tid;
        pLogData->this_fn = pRec[i].this_fn;
        pLogData->call_site = pRec[i].call_site;
        memcpy(pLogData->data, m_strFileNameWithoutExt.GetString(), cb_app_name);
        memcpy(pLogData->data + cb_app_name, pRec[i].fn_name, pRec[i].cb_fn_name);

        if (!pLogData->isValid()) {
            Helpers::ErrMessageBox(TEXT("Record %d is not valid"), i);
            goto Cleanup;
        }

        if (!gArchive.appendNet(pLogData, NULL, true)) {
            Helpers::ErrMessageBox(TEXT("Failed to add record %d"), i);
            goto Cleanup;
        }
        
        //fread(pLogData->data, pLogData->cbData(), 1, m_fp));
        int iNext = i + 1;        
        if (iNext == cRec)
            iNext = 0;
        if (c < cRec - 1) {
            if ((pRec[i].nn + 1) != pRec[iNext].nn) {
                Helpers::ErrMessageBox(TEXT("Bad record secuance %d"), i);
                goto Cleanup;
            }
        }
        i = iNext;
    }
Cleanup:
    delete[] pfuncInfo;
}

#pragma warning(pop)

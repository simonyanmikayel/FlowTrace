#include "stdafx.h"
#include "WorkerThread.h"


WorkerThread::WorkerThread(void)
{
    m_bWorking = false;
    m_hThread = 0;
}

void WorkerThread::SetThreadPriority(int nPriority)
{
	if (m_hThread)
		::SetThreadPriority(m_hThread, nPriority);
}

void WorkerThread::StopWork(const int rettryCount)
{
  if (m_bWorking) {
    m_bWorking = false; // signal end of work
    Terminate();
  }
    if (m_hThread)
    {
		SetEvent(m_hTreadEvent);
		CloseHandle(m_hTreadEvent);
		DWORD dwRes = 0;
        int rettry = 0;
		while (rettry < rettryCount && m_hThread != 0 &&
			WAIT_TIMEOUT == (dwRes = WaitForSingleObject(m_hThread, 10)))
        {
            //MSG msg;
            //while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            //{
            //    TranslateMessage(&msg);
            //    DispatchMessage(&msg);
            //}
			rettry++;
        }
		if (dwRes == WAIT_TIMEOUT) {
			TerminateThread(m_hThread, 0);
			Cleanup();
		}
	}
}

void WorkerThread::Cleanup()
{
	m_bWorking = false;
	CloseHandle(m_hThread);
	m_hThread = NULL;
}

void WorkerThread::StartWork(LPVOID pWorkParam /*=0*/)
{
    StopWork();

    m_pWorkParam = pWorkParam;
    m_hTreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL); //bManualReset = FALSE
    m_hThread = CreateThread(0, 0, ThreadFunk, this, 0, &m_dwTID);
	int i = GetThreadPriority(m_hThread);
    WaitForSingleObject(m_hTreadEvent, INFINITE);
}

DWORD WINAPI WorkerThread::ThreadFunk(LPVOID lpParameter)
{
    WorkerThread* This = (WorkerThread*)lpParameter;

    This->m_bWorking = true;

    SetEvent(This->m_hTreadEvent);
    This->Work(This->m_pWorkParam);

    This->Cleanup();
    return 0;
}

DWORD WorkerThread::SleepThread(DWORD  dwMilliseconds)
{
	return WaitForSingleObject(m_hTreadEvent, dwMilliseconds);
}

void WorkerThread::ResumeThread()
{
	SetEvent(m_hTreadEvent);
}

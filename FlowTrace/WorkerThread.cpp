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

void WorkerThread::StopWork()
{
  if (m_bWorking) {
    m_bWorking = false; // signal end of work
    Terminate();
  }
    if (m_hThread)
    {
		int rettryCount = 0;
		while (rettryCount < 1000 && m_hThread != 0 && WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 0))
        {
            //MSG msg;
            //while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            //{
            //    TranslateMessage(&msg);
            //    DispatchMessage(&msg);
            //}
			//rettryCount++;
            Sleep(0); // alow Worker thread complite its waorks
        }
    }
}

void WorkerThread::StartWork(LPVOID pWorkParam /*=0*/)
{
    StopWork();

    m_pWorkParam = pWorkParam;
    m_hTreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hThread = CreateThread(0, 0, ThreadFunk, this, 0, &m_dwTID);
	int i = GetThreadPriority(m_hThread);
    WaitForSingleObject(m_hTreadEvent, INFINITE);
    CloseHandle(m_hTreadEvent);
}

DWORD WINAPI WorkerThread::ThreadFunk(LPVOID lpParameter)
{
    WorkerThread* This = (WorkerThread*)lpParameter;

    This->m_bWorking = true;

    SetEvent(This->m_hTreadEvent);
    This->Work(This->m_pWorkParam);

    This->m_bWorking = false;
    CloseHandle(This->m_hThread);
    This->m_hThread = NULL;

    return 0;
}

void WorkerThread::OnThreadReady()
{
  SetEvent(m_hTreadEvent);
}

